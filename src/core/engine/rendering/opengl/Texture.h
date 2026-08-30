#ifndef TOOMANYBLOCKS_TEXTURE_H
#define TOOMANYBLOCKS_TEXTURE_H

#include <stddef.h>  // For size_t

#include <string>
#include <vector>

#include "engine/rendering/opengl/RenderApiObject.h"

enum class TextureType {
    Color,
    Depth,
    Float16,
    Float32,
    UInt8,
    UInt16,
    UInt32,
    Int8,
    Int16,
    Int32,
};

enum class TextureFilter {
    Linear,
    Nearest,
};

enum class TextureWrap {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
};

class Texture : public RenderApiObject {
private:
    static thread_local unsigned int currentUnit;
    static thread_local std::vector<unsigned int> currentlyBoundTextures;  // Per unit
    TextureType m_type;
    unsigned int m_width;
    unsigned int m_height;
    int m_channels;

    TextureFilter m_baseFilter;
    TextureFilter m_mipmapFilter;
    TextureWrap m_wrapMode;
    bool m_hasMipmaps;

    Texture(
        TextureType type,
        unsigned int width,
        unsigned int height,
        int channels,
        const void* data,
        TextureFilter filterMode,
        TextureWrap wrapMode
    );

    /**
     * @brief Initialies the cache by reading the maximum texture units given the hardware.
     */
    static void initCache();

public:
    /**
     * @brief Unbinds any currently bound texture from the current active unit.
     */
    static void bindDefault();
    /**
     * @brief Synchronizes the wrapper's internal binding state with OpenGL.
     *
     * Should be used if the Texture binding is changed manually.
     */
    static void syncBinding();
    /**
     * @brief Sets the active unit if it is not already the active one.
     *
     * @param unit The new active texture unit.
     */
    static void setActiveUnit(unsigned int unit);

    /**
     * @return The maximum number of texture units available.
     */
    static size_t getMaxTextureUnits();

    /**
     * Creates a 2D texture with the given specifications.
     *
     * @param type        The type of the texture (Color, Depth, Float16, etc.).
     * @param width       Width of the texture in pixels.
     * @param height      Height of the texture in pixels.
     * @param channels    Number of color channels (e.g., 1 for R, 3 for RGB, 4 for RGBA). Default is 4.
     * @param data        Pointer to the raw pixel data. If nullptr, the texture will allocated but remains
     * uninitialized.
     * @param filterMode  Filter mode for both minification and magnification. Default is Nearest.
     * @param wrapMode    Wrap mode for both U and V directions. Default is Repeat.
     *
     * @throws std::runtime_error if the texture format is unsupported or OpenGL call fails.
     */
    static Texture create(
        TextureType type,
        unsigned int width,
        unsigned int height,
        int channels = 4,
        const void* data = nullptr,
        TextureFilter filterMode = TextureFilter::Nearest,
        TextureWrap wrapMode = TextureWrap::Repeat
    );

    /**
     * @brief Constructs an uninitialized texture with id 0.
     */
    Texture() noexcept
        : m_type(TextureType::Color),
          m_width(0),
          m_height(0),
          m_channels(0),
          m_baseFilter(TextureFilter::Nearest),
          m_mipmapFilter(TextureFilter::Nearest),
          m_wrapMode(TextureWrap::Repeat),
          m_hasMipmaps(false) {}
    Texture(Texture&& other) noexcept;
    virtual ~Texture();

    /**
     * Generates mipmaps for the texture.
     *
     * Updates the minification filter to use mipmaps (based on base and mipmap filters).
     * Has no effect if mipmaps already exist.
     *
     * @throws std::runtime_error if the texture is invalid or of type Depth.
     */
    void createMipmaps();

    /**
     * Updates a subregion of the texture's pixel data.
     *
     * @param xOffset   X offset in the texture to start writing to.
     * @param yOffset   Y offset in the texture to start writing to.
     * @param width     Width of the region to update.
     * @param height    Height of the region to update.
     * @param data      Pointer to the new pixel data.
     *
     * @throws std::runtime_error if texture is invalid, out of bounds, or of unsupported type for the update.
     */
    void updateData(int xOffset, int yOffset, unsigned int width, unsigned int height, const void* data) const;

    /**
     * @brief Reads the entire texture into CPU memory.
     *
     * @param dst Pointer to the destination memory where texture data will be copied into.
     *
     * @throws std::runtime_error If dst is nullptr.
     * @throws std::runtime_error If the texture is uninitialized or has an invalid OpenGL ID.
     *
     * @warning This operation forces a GPU-to-CPU synchronization and can be slow.
     *          Avoid calling this frequently.
     */
    void readData(void* dst) const;

    /**
     * @brief Copies a rectangular region from another texture into this texture (GPU-to-GPU).
     *
     * @param src Reference to the source texture.
     * @param width Width of the region to copy in pixels.
     * @param height Height of the region to copy in pixels.
     * @param srcXOffset X offset in the source texture where copying begins.
     * @param srcYOffset Y offset in the source texture where copying begins.
     * @param dstXOffset X offset in this texture where the copied data will be written.
     * @param dstYOffset Y offset in this texture where the copied data will be written.
     *
     * @throws std::runtime_error If the source or destination regions are out of bounds.
     * @throws std::runtime_error If the source and destination textures are the same.
     * @throws std::runtime_error If the source and destination texture types do not match.
     * @throws std::runtime_error If either texture is uninitialized or has an invalid OpenGL ID.
     *
     * @note This operation performs a GPU-to-GPU copy, avoiding CPU readback.
     */
    void copyDataFrom(
        const Texture& src,
        unsigned int width,
        unsigned int height,
        unsigned int srcXOffset = 0,
        unsigned int srcYOffset = 0,
        unsigned int dstXOffset = 0,
        unsigned int dstYOffset = 0
    ) const;

    /**
     * Binds the texture to the currently active texture unit if not already bound.
     *
     * @throws std::runtime_error if texture is not valid.
     */
    void bind() const;

    /**
     * Binds the texture to the specified texture unit.
     *
     * @param unit  Texture unit (e.g., 0 for GL_TEXTURE0).
     *
     * @throws std::runtime_error if texture is not valid.
     */
    void bindToUnit(unsigned int unit = 0) const;

    /**
     * Sets the base texture filtering mode (min/mag).
     *
     * If mipmaps are enabled, only the magnification filter is updated here.
     *
     * @param filterMode  Either Linear or Nearest.
     */
    void setTextureFilterMode(TextureFilter filterMode);
    /**
     * Sets the minification filter mode used when mipmaps are enabled.
     *
     * This only affects the minification filter (`GL_TEXTURE_MIN_FILTER`). If mipmaps have not
     * been generated yet, the setting will have no effect until `createMipmaps()` is called.
     *
     * @param filterMode  The mipmap filter mode to use (e.g., Nearest or Linear).
     */
    void setMipmapFilterMode(TextureFilter filterMode);
    /**
     * Sets the texture wrapping mode for both the S and T axes.
     *
     * Controls how texture coordinates outside the [0, 1] range are handled. Common modes are
     * `Repeat`, `ClampToEdge`, and `MirroredRepeat`.
     *
     * @param wrapMode  The wrapping mode to apply to both U and V directions.
     */
    void setTextureWrapMode(TextureWrap wrapMode);

    inline TextureType type() const { return m_type; }
    /** @return Texture width in pixels. */
    inline unsigned int width() const { return m_width; }
    /** @return Texture height in pixels. */
    inline unsigned int height() const { return m_height; }
    /** @return Number of color channels. */
    inline int channels() const { return m_channels; }
    /** @return The base texture filter mode (e.g., Nearest or Linear). */
    inline TextureFilter getTextureFilterMode() const { return m_baseFilter; }
    /**
     * Returns the mipmap-specific filter mode used for minification.
     *
     * This is only relevant when mipmaps are generated and enabled.
     *
     * @return The mipmap filter mode.
     */
    inline TextureFilter getMipmapFilterMode() const { return m_mipmapFilter; }
    /** @return The the current texture wrapping mode (e.g., Repeat, ClampToEdge, ...). */
    inline TextureWrap getWrapMode() const { return m_wrapMode; }
    /** @return Whether mipmaps have been generated for this texture. */
    inline bool hasMipmaps() const { return m_hasMipmaps; }

    Texture& operator=(Texture&& other) noexcept;
};

#endif
