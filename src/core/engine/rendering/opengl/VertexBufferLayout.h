#ifndef TOOMANYBLOCKS_VERTEXBUFFERLAYOUT_H
#define TOOMANYBLOCKS_VERTEXBUFFERLAYOUT_H

#include <vector>

struct BufferLayoutElement {
    unsigned int type;
    unsigned int typeSize;
    unsigned int count;
    bool normalized;
};

/**
 * @brief Defines the memory layout of vertex buffer attributes.
 */
class VertexBufferLayout {
private:
    std::vector<BufferLayoutElement> m_elements;
    unsigned int m_stride;

public:
    VertexBufferLayout() : m_stride(0) {};

    /**
     * @brief Adds an attribute to the layout.
     *
     * @param type OpenGL data type (e.g., GL_FLOAT)
     * @param count Number of components for the attribute
     * @param normalized Whether the data should be normalized
     */
    void push(unsigned int type, unsigned int count, bool normalized = false);

    /** @return Returns the list of layout elements. */
    inline const std::vector<BufferLayoutElement>& elements() const { return m_elements; }

    /** @brief Returns the stride (in bytes) between consecutive vertices. */
    inline unsigned int stride() const { return m_stride; }
};

#endif