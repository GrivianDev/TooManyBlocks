#ifndef TOOMANYBLOCKS_SSAOPROCESSOR_H
#define TOOMANYBLOCKS_SSAOPROCESSOR_H

#include <glm/glm.hpp>
#include <memory>

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/opengl/Texture.h"
#include "engine/rendering/opengl/VertexArray.h"
#include "engine/rendering/opengl/VertexBuffer.h"

#define SSAO_SAMPLE_COUNT 32

struct ApplicationContext;

class SSAOProcessor {
private:
    unsigned int m_ssaoBufferWidth;
    unsigned int m_ssaoBufferHeight;
    Shader m_ssaoPassShader;
    Shader m_ssaoBlurShader;
    FrameBuffer m_ssaoGBuffer;
    FrameBuffer m_ssaoPassBuffer;
    FrameBuffer m_ssaoBlurBuffer;
    Texture m_ssaoNoiseTexture;
    glm::vec3* m_ssaoSamples;

    void createBuffers();

public:
    SSAOProcessor();
    ~SSAOProcessor();

    void validateBuffers(const ApplicationContext& context);
    void prepareSSAOGBufferPass(const ApplicationContext& context);
    void prepareSSAOPass(const ApplicationContext& context);
    void prepareSSAOBlurPass(const ApplicationContext& context);

    const Texture* getOcclusionOutput() const;
};

#endif
