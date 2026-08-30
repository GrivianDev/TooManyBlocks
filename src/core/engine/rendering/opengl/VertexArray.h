#ifndef TOOMANYBLOCKS_VERTEXARRAY_H
#define TOOMANYBLOCKS_VERTEXARRAY_H

#include "engine/rendering/opengl/RenderApiObject.h"
#include "engine/rendering/opengl/VertexBuffer.h"

/**
 * @brief Represents an OpenGL Vertex Array Object (VAO).
 *
 * Encapsulates vertex attribute configuration for one or more VertexBuffers. Attributes
 * are assigned incrementally using an internal index and can be reset via resetAttribIndex().
 *
 * Use addBuffer() to bind a VertexBuffer and configure its layout for use with the VAO.
 */
class VertexArray : public RenderApiObject {
private:
    static thread_local unsigned int currentlyBoundVAO;
    unsigned int m_currAttribIndex;

    void internalAddBuffer(const VertexBuffer& vb, unsigned int divisor);

public:
    /**
     * @brief Unbinds any currently bound vertex array.
     */
    static void bindDefault();
    /**
     * @brief Synchronizes the wrapper's internal binding state with OpenGL.
     *
     * Should be used if the VAO binding is changed manually.
     */
    static void syncBinding();
    /**
     * @brief Creates a new vertex array.
     */
    static VertexArray create();

    /**
     * @brief Constructs an uninitialized vertex array with id 0.
     */
    VertexArray() noexcept : m_currAttribIndex(0) {}
    VertexArray(VertexArray&& other) noexcept;
    virtual ~VertexArray();

    /**
     * @brief Adds a vertex buffer to the VAO and configures its vertex attributes.
     *
     * The buffer’s layout must be set prior to this call. Attributes are assigned
     * sequentially starting at the current index. Use resetAttribIndex() to reuse slots.
     *
     * @param vb The VertexBuffer to bind and configure.
     *
     * @throws std::runtime_error If ID is 0 (uninitialized or moved-from), same for the passed vertex buffer.
     */
    void addBuffer(const VertexBuffer& vb);

    /**
     * @brief Adds a vertex buffer to the VAO and configures its vertex attributes for instanced rendering.
     *
     * The buffer’s layout must be set prior to this call. Each attibutes advances once per instance.
     * Attributes are assigned sequentially starting at the current index. Use resetAttribIndex() to reuse slots.
     *
     * @param vb The VertexBuffer to bind and configure for instancing.
     *
     * @throws std::runtime_error If ID is 0 (uninitialized or moved-from), same for the passed vertex buffer.
     */
    void addInstanceBuffer(const VertexBuffer& vb);

    /**
     * @brief  Binds this VAO if not already bound.
     *
     * @throws std::runtime_error If the VAO ID is 0 (uninitialized or moved-from).
     */
    void bind() const;

    /**
     * @brief Resets the attribute index counter to zero.
     *
     * Useful before calling addBuffer() to reuse attribute locations.
     */
    void resetAttribIndex();

    VertexArray& operator=(VertexArray&& other) noexcept;
};

#endif