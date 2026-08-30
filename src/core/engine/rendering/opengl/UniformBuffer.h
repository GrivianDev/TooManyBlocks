#ifndef TOOMANYBLOCKS_UNIFORMBUFFER_H
#define TOOMANYBLOCKS_UNIFORMBUFFER_H

#include <stddef.h>  // For size_t

#include "engine/rendering/opengl/RenderApiObject.h"

/**
 * @brief Represents an OpenGL Uniform Buffer Object (UBO).
 *
 * Encapsulates allocation, binding, and data updates for uniform buffers.
 * Binds buffers via `glBindBufferBase` to a specific binding point.
 */
class UniformBuffer : public RenderApiObject {
private:
    static thread_local unsigned int currentlyBoundUBO;
    size_t m_size;

    UniformBuffer(const void* data, size_t size);

public:
    /**
     * @brief Unbinds any currently bound uniform buffer.
     */
    static void bindDefault();
    /**
     * @brief Synchronizes the wrapper's internal binding state with OpenGL.
     *
     * Should be used if the UBO binding is changed manually.
     */
    static void syncBinding();

    /**
     * @brief Creates a new uniform buffer and uploads initial data.
     *
     * If `data` is `nullptr`, memory is allocated but not initialized.
     *
     * @param data Pointer to the initial buffer contents, or nullptr.
     * @param size Size of the buffer in bytes.
     */
    static UniformBuffer create(const void* data, size_t size);

    /**
     * @brief Constructs an uninitialized uniform buffer with id 0.
     */
    UniformBuffer() noexcept : m_size(0) {}
    UniformBuffer(UniformBuffer&& other) noexcept;
    virtual ~UniformBuffer();

    /**
     * @brief Updates a portion of the uniform buffer with new data.
     *
     * @param data Pointer to the source data.
     * @param size Size of the data in bytes.
     * @param offset Byte offset in the buffer where data should be written.
     *
     * @throws std::runtime_error if the offset + size exceeds the buffer’s size.
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void updateData(const void* data, size_t size, size_t offset = 0) const;

    /**
     * @brief Reads a portion of the uniform buffer into CPU memory.
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
     * @brief Copies data from another uniform buffer into this buffer.
     *
     * @param src Reference to the source UniformBuffer.
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
    void copyDataFrom(const UniformBuffer& src, size_t size, size_t srcOffset = 0, size_t dstOffset = 0) const;

    /**
     * @brief Binds the uniform buffer to GL_UNIFORM_BUFFER if not already bound.
     *
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void bind() const;

    /**
     * @brief Binds the buffer to a specific global uniform buffer binding point for usage in shaders.
     *
     * @param bindingPoint Index in the block binding range.
     */
    void assignTo(unsigned int bindingPoint) const;

    /** @return Returns the size of the buffer in bytes. */
    inline size_t getByteSize() const { return m_size; };

    UniformBuffer& operator=(UniformBuffer&& other) noexcept;
};

#endif