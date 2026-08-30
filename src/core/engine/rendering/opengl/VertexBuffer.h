#ifndef TOOMANYBLOCKS_VERTEXBUFFER_H
#define TOOMANYBLOCKS_VERTEXBUFFER_H

#include <stddef.h>  // For size_t

#include "engine/rendering/opengl/RenderApiObject.h"
#include "engine/rendering/opengl/VertexBufferLayout.h"

/**
 * @brief Represents an OpenGL Vertex Buffer Object (VBO).
 *
 * Manages GPU memory for vertex data. Supports data uploads and layout specification.
 */
class VertexBuffer : public RenderApiObject {
private:
    static thread_local unsigned int currentlyBoundVBO;
    VertexBufferLayout m_layout;
    size_t m_size;

    VertexBuffer(const void* data, size_t size);

public:
    /**
     * @brief Unbinds any currently bound vertex buffer.
     */
    static void bindDefault();
    /**
     * @brief Syncs the internal binding state with the current OpenGL binding.
     *
     * Should be used if VBO binding is changed manually.
     */
    static void syncBinding();

    /**
     * @brief Creates a vertex buffer with optional initial data.
     *
     * If `data` is `nullptr`, memory is allocated but not initialized.
     *
     * @param data Pointer to the vertex data, or nullptr to allocate only.
     * @param size Size of the buffer in bytes.
     */
    static VertexBuffer create(const void* data, size_t size);

    /**
     * @brief Constructs an uninitialized vertex buffer with id 0.
     */
    VertexBuffer() noexcept : m_size(0) {}
    VertexBuffer(VertexBuffer&& other) noexcept;
    virtual ~VertexBuffer();

    /**
     * @brief Updates a portion of the vertex buffer with new data.
     *
     * @param data Pointer to the source data.
     * @param size Size of the data in bytes.
     * @param offset Byte offset in the buffer where data should be written.
     *
     * @throws std::runtime_error If the update exceeds the allocated buffer size.
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void updateData(const void* data, size_t size, size_t offset = 0) const;

    /**
     * @brief Reads a portion of the vertex buffer into CPU memory.
     *
     * @param dst Pointer to the destination memory where buffer data will be copied into.
     * @param size Size of the data to read in bytes.
     * @param offset Byte offset in the buffer from which to start reading.
     *
     * @throws std::runtime_error If the read exceeds the allocated buffer size.
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     *
     * @warning This operation forces a GPU-to-CPU synchronization and can be slow.
     *          Avoid calling this frequently.
     */
    void readData(void* dst, size_t size, size_t offset = 0) const;

    /**
     * @brief Copies data from another vertex buffer into this buffer.
     *
     * @param src Reference to the source VertexBuffer.
     * @param size Size of the data to copy in bytes.
     * @param srcOffset Byte offset in the source buffer from which to start copying.
     * @param dstOffset Byte offset in this buffer where data should be written.
     *
     * @throws std::runtime_error If the source or destination copy region exceeds buffer size.
     * @throws std::runtime_error If this buffer and the source buffer are the same and regions overlap.
     * @throws std::runtime_error If either buffer ID is 0 (uninitialized or moved-from).
     *
     * @note This performs a GPU-to-GPU copy, avoiding CPU readback.
     */
    void copyDataFrom(const VertexBuffer& src, size_t size, size_t srcOffset = 0, size_t dstOffset = 0) const;

    /**
     * @brief Completely fills vertex buffer memory with zero. May discard old buffer.
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void clearData() const;

    /**
     * @brief Binds the vertex buffer to GL_ELEMENT_ARRAY_BUFFER if not already bound.
     *
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void bind() const;

    /**
     * @brief Sets the layout describing the structure of the vertex data.
     */
    inline void setLayout(const VertexBufferLayout& layout) { m_layout = layout; }

    /**
     * @brief Returns the currently assigned vertex buffer layout.
     */
    inline const VertexBufferLayout& getLayout() const { return m_layout; }

    /** @return The total size of the buffer in bytes. */
    inline size_t getByteSize() const { return m_size; };

    /**
     * @return The number of vertices stored in the buffer. This is derived from the set layout
     * and is 0 if no layout is set.
     */
    inline size_t getVertexCount() const { return m_layout.stride() ? (m_size / m_layout.stride()) : 0; }

    VertexBuffer& operator=(VertexBuffer&& other) noexcept;
};

#endif