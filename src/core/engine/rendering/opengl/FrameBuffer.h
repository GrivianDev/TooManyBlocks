#ifndef TOOMANYBLOCKS_FRAMEBUFFER_H
#define TOOMANYBLOCKS_FRAMEBUFFER_H

#include <memory>
#include <vector>

#include "engine/rendering/opengl/RenderApiObject.h"
#include "engine/rendering/opengl/Texture.h"

/**
 * @brief Represents an OpenGL Framebuffer Object (FBO).
 *
 * Allows attaching color and depth textures and managing rendering output targets.
 */
class FrameBuffer : public RenderApiObject {
private:
    static thread_local unsigned int currentlyBoundFBO;
    std::vector<std::shared_ptr<Texture>> m_attachedTextures;
    std::shared_ptr<Texture> m_attachedDepthTexture;

    /**
     * @brief Finalizes draw buffer setup based on attached textures.
     *
     * Configures the list of draw buffers for the framebuffer or
     * disables both drawing and reading if no color attachments are present.
     *
     * This must be called after attaching or clearing textures to ensure correct
     * rendering behavior and framebuffer completeness.
     */
    void finalizeDrawBufferOutput();

public:
    /**
     * @brief Binds the default framebuffer (id 0).
     *
     * Use this to return rendering output back to the default window surface.
     */
    static void bindDefault();
    /**
     * @brief Synchronizes the wrapper's internal binding state with OpenGL.
     *
     * Should be used if the FBO binding is changed manually.
     */
    static void syncBinding();

    /**
     * @return A new FrameBuffer instance with a valid renderer ID.
     */
    static FrameBuffer create();

    /**
     * @brief Constructs an uninitialized frame buffer with id 0.
     */
    FrameBuffer() noexcept = default;
    FrameBuffer(FrameBuffer&& other) noexcept;
    virtual ~FrameBuffer();

    /**
     * @brief Binds this framebuffer for rendering.
     *
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void bind() const;

    /**
     * @brief Attaches a texture to the framebuffer.
     *
     * Attaches to the next available color attachment point, or as depth if texture is a depth texture.
     *
     * @param texture The texture to attach.
     */
    void attachTexture(std::shared_ptr<Texture> texture);

    /**
     * @brief Removes all currently attached color and depth textures from this frame buffer.
     */
    void clearAttachedTextures();

    /**
     * @return Reference to the list of attached color textures.
     */
    inline const std::vector<std::shared_ptr<Texture>>& getAttachedTextures() const { return m_attachedTextures; }

    /**
     * @return Shared pointer to the depth texture, or nullptr if none is attached.
     */
    inline std::shared_ptr<Texture> getAttachedDepthTexture() const { return m_attachedDepthTexture; }

    FrameBuffer& operator=(FrameBuffer&& other) noexcept;
};

#endif