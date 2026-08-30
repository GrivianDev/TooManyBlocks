#include "VertexArray.h"

#include <GL/glew.h>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"

thread_local unsigned int VertexArray::currentlyBoundVAO = 0;

constexpr bool isIntegerBased(unsigned int type) {
    return type == GL_INT || type == GL_UNSIGNED_INT || type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
           type == GL_BYTE || type == GL_UNSIGNED_BYTE;
}

void VertexArray::internalAddBuffer(const VertexBuffer& vb, unsigned int divisor) {
    bind();
    vb.bind();
    size_t offset = 0;
    for (const BufferLayoutElement& element : vb.getLayout().elements()) {
        GLCALL(glEnableVertexAttribArray(m_currAttribIndex));
        if (isIntegerBased(element.type)) {
            GLCALL(glVertexAttribIPointer(
                m_currAttribIndex, element.count, element.type, vb.getLayout().stride(), (const void*)offset
            ));
        } else {
            GLCALL(glVertexAttribPointer(
                m_currAttribIndex, element.count, element.type, element.normalized ? GL_TRUE : GL_FALSE,
                vb.getLayout().stride(), (const void*)offset
            ));
        }
        if (divisor != 0) {
            // Attrib advances every "divisor" instances compared to per vertex default
            GLCALL(glVertexAttribDivisor(m_currAttribIndex, divisor));
        }
        offset += element.count * element.typeSize;
        m_currAttribIndex++;
    }
}

void VertexArray::bindDefault() {
    if (VertexArray::currentlyBoundVAO != 0) {
        GLCALL(glBindVertexArray(0));
        VertexArray::currentlyBoundVAO = 0;
    }
}

void VertexArray::syncBinding() {
    int binding;
    GLCALL(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &binding));
    VertexArray::currentlyBoundVAO = static_cast<unsigned int>(binding);
}

VertexArray VertexArray::create() {
    VertexArray array;
    GLCALL(glGenVertexArrays(1, &array.m_rendererId));
    return array;
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : RenderApiObject(std::move(other)), m_currAttribIndex(other.m_currAttribIndex) {}

VertexArray::~VertexArray() {
    if (isValid()) {
        try {
            if (VertexArray::currentlyBoundVAO == m_rendererId) {
                GLCALL(glBindVertexArray(0));
                VertexArray::currentlyBoundVAO = 0;
            }
            GLCALL(glDeleteVertexArrays(1, &m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during VertexArray cleanup");
        }
    }
}

void VertexArray::addBuffer(const VertexBuffer& vb) {
    internalAddBuffer(vb, 0);
}

void VertexArray::addInstanceBuffer(const VertexBuffer& vb) {
    internalAddBuffer(vb, 1);
}

void VertexArray::bind() const {
    if (!isValid()) throw std::runtime_error("Invalid state of VertexArray with id 0");

    if (VertexArray::currentlyBoundVAO != m_rendererId) {
        GLCALL(glBindVertexArray(m_rendererId));
        VertexArray::currentlyBoundVAO = m_rendererId;
    }
}

void VertexArray::resetAttribIndex() { m_currAttribIndex = 0; }

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            try {
                if (VertexArray::currentlyBoundVAO == m_rendererId) {
                    GLCALL(glBindVertexArray(0));
                    VertexArray::currentlyBoundVAO = 0;
                }
                GLCALL(glDeleteVertexArrays(1, &m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during VertexArray cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));

        m_currAttribIndex = other.m_currAttribIndex;
    }
    return *this;
}