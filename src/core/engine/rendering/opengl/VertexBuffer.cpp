#include "VertexBuffer.h"

#include <GL/glew.h>

#include <cstring>
#include <stdexcept>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"

thread_local unsigned int VertexBuffer::currentlyBoundVBO = 0;

VertexBuffer::VertexBuffer(const void* data, size_t size) : m_size(size) {
    GLCALL(glGenBuffers(1, &m_rendererId));
    bind();
    GLCALL(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer::bindDefault() {
    if (VertexBuffer::currentlyBoundVBO != 0) {
        GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
        VertexBuffer::currentlyBoundVBO = 0;
    }
}

void VertexBuffer::syncBinding() {
    int binding;
    GLCALL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &binding));
    VertexBuffer::currentlyBoundVBO = static_cast<unsigned int>(binding);
}

VertexBuffer VertexBuffer::create(const void* data, size_t size) { return VertexBuffer(data, size); }

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : RenderApiObject(std::move(other)), m_layout(std::move(other.m_layout)), m_size(other.m_size) {}

VertexBuffer::~VertexBuffer() {
    if (isValid()) {
        try {
            if (VertexBuffer::currentlyBoundVBO == m_rendererId) {
                GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
                VertexBuffer::currentlyBoundVBO = 0;
            }
            GLCALL(glDeleteBuffers(1, &m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during VertexBuffer cleanup");
        }
    }
}

void VertexBuffer::updateData(const void* data, size_t size, size_t offset) const {
    bind();

    if (offset + size > m_size) throw std::runtime_error("VBO update exceeds buffer size");

    GLCALL(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexBuffer::readData(void* dst, size_t size, size_t offset) const {
    bind();

    if (!dst) throw std::runtime_error("VBO read destination is nullptr");
    if (offset + size > m_size) throw std::runtime_error("VBO read exceeds buffer size");

    GLCALL(void* src = glMapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_READ_BIT));

    if (!src) throw std::runtime_error("Failed to map VBO for reading");

    std::memcpy(dst, src, size);

    GLCALL(glUnmapBuffer(GL_ARRAY_BUFFER));
}

void VertexBuffer::copyDataFrom(const VertexBuffer& src, size_t size, size_t srcOffset, size_t dstOffset) const {
    size_t srcEnd = srcOffset + size;
    size_t dstEnd = dstOffset + size;
    if (srcEnd > src.m_size) throw std::runtime_error("Source VBO copy exceeds buffer size");
    if (dstEnd > m_size) throw std::runtime_error("Destination VBO copy exceeds buffer size");
    if (&src == this && srcOffset < dstEnd && dstOffset < srcEnd) throw std::runtime_error("Overlapping VBO copy");

    GLCALL(glBindBuffer(GL_COPY_READ_BUFFER, src.m_rendererId));
    GLCALL(glBindBuffer(GL_COPY_WRITE_BUFFER, m_rendererId));

    GLCALL(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, dstOffset, size));
}

void VertexBuffer::clearData() const {
    bind();
    GLCALL(void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, m_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
    std::memset(ptr, 0, m_size);
    GLCALL(glUnmapBuffer(GL_ARRAY_BUFFER));
}

void VertexBuffer::bind() const {
    if (!isValid()) throw std::runtime_error("Invalid state of VertexBuffer with id 0");

    if (VertexBuffer::currentlyBoundVBO != m_rendererId) {
        GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_rendererId));
        VertexBuffer::currentlyBoundVBO = m_rendererId;
    }
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            try {
                if (VertexBuffer::currentlyBoundVBO == m_rendererId) {
                    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
                    VertexBuffer::currentlyBoundVBO = 0;
                }
                GLCALL(glDeleteBuffers(1, &m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during VertexBuffer cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));
        m_layout = std::move(other.m_layout);
        m_size = other.m_size;
    }
    return *this;
}