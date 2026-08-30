#include "IndexBuffer.h"

#include <GL/glew.h>

#include <stdexcept>
#include <cstring>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"

thread_local unsigned int IndexBuffer::currentlyBoundIBO = 0;

IndexBuffer::IndexBuffer(const unsigned int* data, size_t count) : m_count(count) {
    GLCALL(glGenBuffers(1, &m_rendererId));
    bind();
    GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

void IndexBuffer::bindDefault() {
    if (IndexBuffer::currentlyBoundIBO != 0) {
        GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        IndexBuffer::currentlyBoundIBO = 0;
    }
}

void IndexBuffer::syncBinding() {
    int binding;
    GLCALL(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding));
    IndexBuffer::currentlyBoundIBO = static_cast<unsigned int>(binding);
}

IndexBuffer IndexBuffer::create(const unsigned int* data, size_t count) { return IndexBuffer(data, count); }

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept : RenderApiObject(std::move(other)), m_count(other.m_count) {}

IndexBuffer::~IndexBuffer() {
    if (isValid()) {
        try {
            if (IndexBuffer::currentlyBoundIBO == m_rendererId) {
                GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                IndexBuffer::currentlyBoundIBO = 0;
            }
            GLCALL(glDeleteBuffers(1, &m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during IndexBuffer cleanup");
        }
    }
}

void IndexBuffer::updateData(const unsigned int* data, size_t count, size_t offset) const {
    bind();

    if (offset + count > m_count) throw std::runtime_error("VBO update exceeds buffer size");

    GLCALL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, count * sizeof(unsigned int), data));
}

void IndexBuffer::readData(unsigned int* dst, size_t count, size_t offset) const {
    bind();

    if (!dst) throw std::runtime_error("IBO read destination is nullptr");
    if (offset + count > m_count) throw std::runtime_error("IBO read exceeds buffer size");

    GLCALL(
        void* src = glMapBufferRange(
            GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(unsigned int), count * sizeof(unsigned int), GL_MAP_READ_BIT
        )
    );

    if (!src) throw std::runtime_error("Failed to map IBO for reading");

    std::memcpy(dst, src, count * sizeof(unsigned int));

    GLCALL(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));
}

void IndexBuffer::copyDataFrom(const IndexBuffer& src, size_t count, size_t srcOffset, size_t dstOffset) const {
    size_t srcEnd = srcOffset + count;
    size_t dstEnd = dstOffset + count;
    if (srcEnd > src.m_count) throw std::runtime_error("Source IBO copy exceeds buffer size");
    if (dstEnd > m_count) throw std::runtime_error("Destination IBO copy exceeds buffer size");
    if (&src == this && srcOffset < dstEnd && dstOffset < srcEnd) throw std::runtime_error("Overlapping IBO copy");

    GLCALL(glBindBuffer(GL_COPY_READ_BUFFER, src.m_rendererId));
    GLCALL(glBindBuffer(GL_COPY_WRITE_BUFFER, m_rendererId));

    GLCALL(glCopyBufferSubData(
        GL_COPY_READ_BUFFER,
        GL_COPY_WRITE_BUFFER,
        srcOffset * sizeof(unsigned int),
        dstOffset * sizeof(unsigned int),
        count * sizeof(unsigned int)
    ));
}

void IndexBuffer::bind() const {
    if (!isValid()) throw std::runtime_error("Invalid state of IndexBuffer with id 0");

    if (IndexBuffer::currentlyBoundIBO != m_rendererId) {
        GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId));
        IndexBuffer::currentlyBoundIBO = m_rendererId;
    }
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            try {
                if (IndexBuffer::currentlyBoundIBO == m_rendererId) {
                    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                    IndexBuffer::currentlyBoundIBO = 0;
                }
                GLCALL(glDeleteBuffers(1, &m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during IndexBuffer cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));
        m_count = other.m_count;
    }
    return *this;
}
