#include "UniformBuffer.h"

#include <GL/glew.h>

#include <cstring>
#include <stdexcept>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"

thread_local unsigned int UniformBuffer::currentlyBoundUBO = 0;

UniformBuffer::UniformBuffer(const void* data, size_t size) : m_size(size) {
    GLCALL(glGenBuffers(1, &m_rendererId));
    bind();
    GLCALL(glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

void UniformBuffer::bindDefault() {
    if (UniformBuffer::currentlyBoundUBO != 0) {
        GLCALL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
        UniformBuffer::currentlyBoundUBO = 0;
    }
}

void UniformBuffer::syncBinding() {
    int binding;
    GLCALL(glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &binding));
    UniformBuffer::currentlyBoundUBO = static_cast<unsigned int>(binding);
}

UniformBuffer UniformBuffer::create(const void* data, size_t size) { return UniformBuffer(data, size); }

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : RenderApiObject(std::move(other)), m_size(other.m_size) {}

UniformBuffer::~UniformBuffer() {
    if (isValid()) {
        try {
            if (UniformBuffer::currentlyBoundUBO == m_rendererId) {
                GLCALL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
                UniformBuffer::currentlyBoundUBO = 0;
            }
            GLCALL(glDeleteBuffers(1, &m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during UniformBuffer cleanup");
        }
    }
}

void UniformBuffer::updateData(const void* data, size_t size, size_t offset) const {
    bind();

    if (offset + size > m_size) throw std::runtime_error("UBO update exceeds buffer size");

    GLCALL(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
}

void UniformBuffer::readData(void* dst, size_t size, size_t offset) const {
    bind();

    if (!dst) throw std::runtime_error("UBO read destination is nullptr");
    if (offset + size > m_size) throw std::runtime_error("UBO read exceeds buffer size");
    
    GLCALL(void* src = glMapBufferRange(GL_UNIFORM_BUFFER, offset, size, GL_MAP_READ_BIT));
    if (!src) throw std::runtime_error("Failed to map UBO for reading");

    std::memcpy(dst, src, size);

    GLCALL(glUnmapBuffer(GL_UNIFORM_BUFFER));
}

void UniformBuffer::copyDataFrom(const UniformBuffer& src, size_t size, size_t srcOffset, size_t dstOffset) const {
    size_t srcEnd = srcOffset + size;
    size_t dstEnd = dstOffset + size;
    if (srcEnd > src.m_size) throw std::runtime_error("Source UBO copy exceeds buffer size");
    if (dstEnd > m_size) throw std::runtime_error("Destination UBO copy exceeds buffer size");
    if (&src == this && srcOffset < dstEnd && dstOffset < srcEnd) throw std::runtime_error("Overlapping UBO copy");

    GLCALL(glBindBuffer(GL_COPY_READ_BUFFER, src.m_rendererId));
    GLCALL(glBindBuffer(GL_COPY_WRITE_BUFFER, m_rendererId));

    GLCALL(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, dstOffset, size));
}

void UniformBuffer::bind() const {
    if (!isValid()) throw std::runtime_error("Invalid state of UniformBuffer with id 0");

    if (UniformBuffer::currentlyBoundUBO != m_rendererId) {
        GLCALL(glBindBuffer(GL_UNIFORM_BUFFER, m_rendererId));
        UniformBuffer::currentlyBoundUBO = m_rendererId;
    }
}

void UniformBuffer::assignTo(unsigned int bindingPoint) const {
    if (!isValid()) throw std::runtime_error("Invalid state of UniformBuffer with id 0");

    GLCALL(glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_rendererId));
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            try {
                if (UniformBuffer::currentlyBoundUBO == m_rendererId) {
                    GLCALL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
                    UniformBuffer::currentlyBoundUBO = 0;
                }
                GLCALL(glDeleteBuffers(1, &m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during UniformBuffer cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));

        m_size = other.m_size;
    }
    return *this;
}