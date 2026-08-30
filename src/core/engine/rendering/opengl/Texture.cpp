#include "Texture.h"

#include <GL/glew.h>

#include <stdexcept>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"

struct TextureFormat {
    GLenum internalFormat;
    GLenum inputFormat;
    GLenum type;
};

thread_local unsigned int Texture::currentUnit = 0;

thread_local std::vector<unsigned int> Texture::currentlyBoundTextures;

static constexpr TextureFormat toOpenGLTexFormat(TextureType type, int channels) {
    switch (type) {
        case TextureType::Color:  // Normalized
            switch (channels) {
                case 1: return {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
                case 2: return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
                case 3: return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
                default: return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
            }
        case TextureType::Float32:
            switch (channels) {
                case 1: return {GL_R32F, GL_RED, GL_FLOAT};
                case 2: return {GL_RG32F, GL_RG, GL_FLOAT};
                case 3: return {GL_RGB32F, GL_RGB, GL_FLOAT};
                default: return {GL_RGBA32F, GL_RGBA, GL_FLOAT};
            }
        case TextureType::Float16:
            switch (channels) {
                case 1: return {GL_R16F, GL_RED, GL_HALF_FLOAT};
                case 2: return {GL_RG16F, GL_RG, GL_HALF_FLOAT};
                case 3: return {GL_RGB16F, GL_RGB, GL_HALF_FLOAT};
                default: return {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT};
            }
        case TextureType::UInt8:
            switch (channels) {
                case 1: return {GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE};
                case 2: return {GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE};
                case 3: return {GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE};
                default: return {GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE};
            }
        case TextureType::UInt16:
            switch (channels) {
                case 1: return {GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT};
                case 2: return {GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT};
                case 3: return {GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT};
                default: return {GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT};
            }
        case TextureType::UInt32:
            switch (channels) {
                case 1: return {GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT};
                case 2: return {GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT};
                case 3: return {GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT};
                default: return {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT};
            }
        case TextureType::Int8:
            switch (channels) {
                case 1: return {GL_R8I, GL_RED_INTEGER, GL_BYTE};
                case 2: return {GL_RG8I, GL_RG_INTEGER, GL_BYTE};
                case 3: return {GL_RGB8I, GL_RGB_INTEGER, GL_BYTE};
                default: return {GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE};
            }
        case TextureType::Int16:
            switch (channels) {
                case 1: return {GL_R16I, GL_RED_INTEGER, GL_SHORT};
                case 2: return {GL_RG16I, GL_RG_INTEGER, GL_SHORT};
                case 3: return {GL_RGB16I, GL_RGB_INTEGER, GL_SHORT};
                default: return {GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT};
            }
        case TextureType::Int32:
            switch (channels) {
                case 1: return {GL_R32I, GL_RED_INTEGER, GL_INT};
                case 2: return {GL_RG32I, GL_RG_INTEGER, GL_INT};
                case 3: return {GL_RGB32I, GL_RGB_INTEGER, GL_INT};
                default: return {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT};
            }
        case TextureType::Depth: return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT};
        default: return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};  // Default case
    }
}

static constexpr GLenum toOpenGLFilterMode(TextureFilter filterMode) {
    if (filterMode == TextureFilter::Linear) {
        return GL_LINEAR;
    }
    return GL_NEAREST;
}

static constexpr GLenum toOpenGLMinFilterModeWithMipmap(TextureFilter baseFilterMode, TextureFilter mipmapFilterMode) {
    // Base filter + mipmap filter config param (Only used for GL_TEXTURE_MIN_FILTER parameter meaning minification)
    if (baseFilterMode == TextureFilter::Linear) {
        if (mipmapFilterMode == TextureFilter::Linear) {
            return GL_LINEAR_MIPMAP_LINEAR;
        } else if (mipmapFilterMode == TextureFilter::Nearest) {
            return GL_LINEAR_MIPMAP_NEAREST;
        }
    } else if (baseFilterMode == TextureFilter::Nearest) {
        if (mipmapFilterMode == TextureFilter::Linear) {
            return GL_NEAREST_MIPMAP_LINEAR;
        } else if (mipmapFilterMode == TextureFilter::Nearest) {
            return GL_NEAREST_MIPMAP_NEAREST;
        }
    }

    // Should never reach here
    return GL_LINEAR_MIPMAP_LINEAR;
}

static constexpr GLenum toOpenGLWrapMode(TextureWrap wrapMode) {
    switch (wrapMode) {
        case TextureWrap::Repeat: return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
        default: return GL_REPEAT;
    }
}

Texture::Texture(
    TextureType type,
    unsigned int width,
    unsigned int height,
    int channels,
    const void* data,
    TextureFilter filterMode,
    TextureWrap wrapMode
)
    : m_type(type),
      m_width(width),
      m_height(height),
      m_channels(channels),
      m_baseFilter(filterMode),
      m_mipmapFilter(TextureFilter::Nearest),
      m_wrapMode(wrapMode),
      m_hasMipmaps(false) {
    GLCALL(glGenTextures(1, &m_rendererId));
    bind();

    // Configure texture
    TextureFormat format = toOpenGLTexFormat(type, channels);
    GLCALL(glTexImage2D(
        GL_TEXTURE_2D, 0, format.internalFormat, m_width, m_height, 0, format.inputFormat, format.type, data
    ));

    // Filter and wrap mode
    GLenum filterParam = toOpenGLFilterMode(filterMode);
    GLenum wrapParam = toOpenGLWrapMode(wrapMode);
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam));
    GLCALL(
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam)
    );  // Note: Depth textures ignore mag filter
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam));
}

void Texture::initCache() {
    // Init cache if empty
    int maxUnits = 0;
    GLCALL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits));
    currentlyBoundTextures.resize(maxUnits);

    if (maxUnits <= 0) {
        lgr::lout.warn("No active texture unit can be set");
    }
}

void Texture::bindDefault() {
    if (currentlyBoundTextures.empty()) {
        Texture::initCache();
    }
    if (Texture::currentlyBoundTextures[Texture::currentUnit] != 0) {
        GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
        Texture::currentlyBoundTextures[Texture::currentUnit] = 0;
    }
}

void Texture::syncBinding() {
    int activeUnit;
    GLCALL(glGetIntegerv(GL_ACTIVE_TEXTURE, &activeUnit));
    Texture::currentUnit = static_cast<unsigned int>(activeUnit - GL_TEXTURE0);

    // For all units check bound textures
    for (unsigned unit = 0; unit < Texture::getMaxTextureUnits(); unit++) {
        GLCALL(glActiveTexture(GL_TEXTURE0 + unit));

        int binding;
        GLCALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding));
        Texture::currentlyBoundTextures[unit] = static_cast<unsigned int>(binding);
    }
    GLCALL(glActiveTexture(GL_TEXTURE0 + Texture::currentUnit));  // Restore active unit
}

void Texture::setActiveUnit(unsigned int unit) {
    if (currentlyBoundTextures.empty()) {
        Texture::initCache();
    }
    if (unit >= Texture::currentlyBoundTextures.size()) {
        throw std::runtime_error(
            "Texture unit " + std::to_string(unit) + " is out of range of maximum total texture units"
        );
    }
    if (currentUnit != unit) {
        GLCALL(glActiveTexture(GL_TEXTURE0 + unit));
        currentUnit = unit;
    }
}

size_t Texture::getMaxTextureUnits() {
    if (currentlyBoundTextures.empty()) {
        Texture::initCache();
    }
    return Texture::currentlyBoundTextures.size();
}

Texture Texture::create(
    TextureType type,
    unsigned int width,
    unsigned int height,
    int channels,
    const void* data,
    TextureFilter filterMode,
    TextureWrap wrapMode
) {
    return Texture(type, width, height, channels, data, filterMode, wrapMode);
}

Texture::Texture(Texture&& other) noexcept
    : RenderApiObject(std::move(other)),
      m_type(other.m_type),
      m_width(other.m_width),
      m_height(other.m_height),
      m_channels(other.m_channels),
      m_baseFilter(other.m_baseFilter),
      m_mipmapFilter(other.m_mipmapFilter),
      m_wrapMode(other.m_wrapMode),
      m_hasMipmaps(other.m_hasMipmaps) {}

Texture::~Texture() {
    if (isValid()) {
        try {
            // Unbind from all units
            for (int unit = 0; unit < Texture::currentlyBoundTextures.size(); unit++) {
                if (Texture::currentlyBoundTextures[unit] == m_rendererId) {
                    Texture::setActiveUnit(unit);
                    GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
                    Texture::currentlyBoundTextures[unit] = 0;
                }
            }
            GLCALL(glDeleteTextures(1, &m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during Texture cleanup");
        }
    }
}

void Texture::createMipmaps() {
    bind();

    if (m_type == TextureType::Depth) throw std::runtime_error("Mipmaps are not supported for depth textures");

    if (!m_hasMipmaps) {
        GLCALL(glGenerateMipmap(GL_TEXTURE_2D));

        // Update minification filter so that mipmaps take effect
        GLCALL(glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toOpenGLMinFilterModeWithMipmap(m_baseFilter, m_mipmapFilter)
        ));
        m_hasMipmaps = true;
    }
}

void Texture::updateData(int xOffset, int yOffset, unsigned int width, unsigned int height, const void* data) const {
    bind();

    if (m_type == TextureType::Depth)
        throw std::runtime_error(
            "Depth textures should ideally not be updated, if still needed then do so manually and unbind it from "
            "active frame buffers"
        );
    if (width + xOffset > m_width || height + yOffset > m_height)
        throw std::runtime_error(
            "Texture update failed: Update region " + std::to_string(width) + "x" + std::to_string(height) +
            " at offset (" + std::to_string(xOffset) + ", " + std::to_string(yOffset) +
            ") exceeds texture dimensions (" + std::to_string(m_width) + "x" + std::to_string(m_height) + ")."
        );

    TextureFormat format = toOpenGLTexFormat(m_type, m_channels);
    GLCALL(glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, width, height, format.inputFormat, format.type, data));
}

void Texture::readData(void* dst) const {
    if (!dst) throw std::runtime_error("Texture read destination is nullptr");

    bind();

    TextureFormat format = toOpenGLTexFormat(m_type, m_channels);

    GLCALL(glGetTexImage(GL_TEXTURE_2D, 0, format.inputFormat, format.type, dst));
}

void Texture::copyDataFrom(
    const Texture& src,
    unsigned int width,
    unsigned int height,
    unsigned int srcXOffset,
    unsigned int srcYOffset,
    unsigned int dstXOffset,
    unsigned int dstYOffset
) const {
    if (&src == this) throw std::runtime_error("Cannot copy texture to itself");

    if (src.m_type != m_type) throw std::runtime_error("Texture type mismatch for copy");

    if (srcXOffset + width > src.m_width || srcYOffset + height > src.m_height)
        throw std::runtime_error("Source texture copy region out of bounds");

    if (dstXOffset + width > m_width || dstYOffset + height > m_height)
        throw std::runtime_error("Destination texture copy region out of bounds");

    GLCALL(glCopyImageSubData(
        src.m_rendererId,
        GL_TEXTURE_2D,
        0,  // src mip level
        srcXOffset,
        srcYOffset,
        0,  // z offset for 2D texture
        m_rendererId,
        GL_TEXTURE_2D,
        0,  // dst mip level
        dstXOffset,
        dstYOffset,
        0,  // z offset for 2D texture
        width,
        height,
        1  // depth = 1 for 2D textures
    ));
}

void Texture::bind() const {
    if (!isValid()) throw std::runtime_error("Invalid state of Texture with id 0");
    if (Texture::currentlyBoundTextures.empty()) {
        Texture::initCache();
    }

    if (Texture::currentlyBoundTextures[Texture::currentUnit] != m_rendererId) {
        GLCALL(glBindTexture(GL_TEXTURE_2D, m_rendererId));
        Texture::currentlyBoundTextures[Texture::currentUnit] = m_rendererId;
    }
}

void Texture::bindToUnit(unsigned int unit) const {
    if (!isValid()) throw std::runtime_error("Invalid state of Texture with id 0");
    Texture::setActiveUnit(unit);
    bind();
}

void Texture::setTextureFilterMode(TextureFilter filterMode) {
    bind();

    if (m_baseFilter != filterMode) {
        m_baseFilter = filterMode;
        // Update min filter taking mipmaps into account if they are generated
        GLCALL(glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            m_hasMipmaps ? toOpenGLMinFilterModeWithMipmap(m_baseFilter, m_mipmapFilter)
                         : toOpenGLFilterMode(m_baseFilter)
        ));
        // Update mag filter (always just nearest or linear - mipmaps have no effect here)
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toOpenGLFilterMode(m_baseFilter)));
    }
}

void Texture::setMipmapFilterMode(TextureFilter filterMode) {
    bind();

    bool changed = m_mipmapFilter != filterMode;
    m_mipmapFilter = filterMode;  // Still store change in the case of createMipmaps() beeing called after this setter
                                  // instead of before
    if (m_hasMipmaps && changed) {
        GLCALL(glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toOpenGLMinFilterModeWithMipmap(m_baseFilter, m_mipmapFilter)
        ));  // Mip mapping
    }
}

void Texture::setTextureWrapMode(TextureWrap wrapMode) {
    bind();

    if (m_wrapMode != wrapMode) {
        m_wrapMode = wrapMode;
        GLenum wrapParam = toOpenGLWrapMode(m_wrapMode);
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam));
    }
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            try {
                // Unbind from all units
                for (int unit = 0; unit < Texture::currentlyBoundTextures.size(); unit++) {
                    if (Texture::currentlyBoundTextures[unit] == m_rendererId) {
                        Texture::setActiveUnit(unit);
                        GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
                        Texture::currentlyBoundTextures[unit] = 0;
                    }
                }
                GLCALL(glDeleteTextures(1, &m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during Texture cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));

        m_type = other.m_type;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_baseFilter = other.m_baseFilter;
        m_mipmapFilter = other.m_mipmapFilter;
        m_wrapMode = other.m_wrapMode;
        m_hasMipmaps = other.m_hasMipmaps;
    }
    return *this;
}
