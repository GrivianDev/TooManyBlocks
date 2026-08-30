#include "VertexBufferLayout.h"

#include "engine/rendering/GLUtils.h"

void VertexBufferLayout::push(unsigned int type, unsigned int count, bool normalized) {
    size_t typeSize = GLsizeof(type);
    m_elements.push_back({type, static_cast<unsigned int>(typeSize), count, normalized});
    m_stride += count * typeSize;
}