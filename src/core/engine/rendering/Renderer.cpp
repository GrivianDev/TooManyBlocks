#include "Renderer.h"

#include <GL/glew.h>

#include <chrono>

#include "Application.h"
#include "Logger.h"
#include "engine/GameInstance.h"
#include "engine/rendering/Camera.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/renderpasses/FXAARenderpass.h"
#include "engine/rendering/renderpasses/OpaqueRenderpass.h"
#include "engine/rendering/renderpasses/ResolverRenderpass.h"
#include "engine/rendering/renderpasses/SSAORenderpass.h"
#include "engine/rendering/renderpasses/ShadowRenderpass.h"
#include "engine/rendering/renderpasses/TransformFeebackpass.h"
#include "engine/rendering/renderpasses/TransparencyRenderpass.h"

static constexpr float fullScreenQuadCCW[] = {
    // Position   // UV-Coords
    -1.0f, 1.0f,  0.0f, 1.0f,  // Top-Left
    -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom-Left
    1.0f,  -1.0f, 1.0f, 0.0f,  // Bottom-Right

    -1.0f, 1.0f,  0.0f, 1.0f,  // Top-Left
    1.0f,  -1.0f, 1.0f, 0.0f,  // Bottom-Right
    1.0f,  1.0f,  1.0f, 1.0f   // Top-Right
};

static constexpr float fullScreenQuadCW[] = {
    // Position   // UV-Coords
    1.0f,  -1.0f, 1.0f, 0.0f,  // Bottom-Right
    -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom-Left
    -1.0f, 1.0f,  0.0f, 1.0f,  // Top-Links

    1.0f,  -1.0f, 1.0f, 0.0f,  // Bottom-Right
    -1.0f, 1.0f,  0.0f, 1.0f,  // Top-Left
    1.0f,  1.0f,  1.0f, 1.0f   // Top-Right
};

void Renderer::init() {
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Error initializing glew!");
    }

    // Graphic api details
    std::ostringstream detailsBuf;
    detailsBuf << "Open GL Version: " << glGetString(GL_VERSION) << "\n";
    detailsBuf << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    detailsBuf << "Graphics: " << glGetString(GL_RENDERER) << "[" << glGetString(GL_VENDOR) << "]" << "\n";

    // GLEnableDebugging();

    GLCALL(glEnable(GL_DEPTH_TEST));
    GLCALL(glEnable(GL_CULL_FACE));  // Enable face culling
    GLCALL(glCullFace(GL_BACK));     // Specify that back faces should be culled (not rendered)
    GLCALL(glFrontFace(GL_CW));      // Specify frontfaces as faces with clockwise winding
    GLCALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

    std::unique_ptr<TransformFeedbackpass> transformFeebackpass = std::make_unique<TransformFeedbackpass>();
    std::unique_ptr<ShadowRenderpass> shadowpass = std::make_unique<ShadowRenderpass>();
    std::unique_ptr<SSAORenderpass> ssaoRenderpass = std::make_unique<SSAORenderpass>();
    std::unique_ptr<OpaqueRenderpass> opaqueRenderpass = std::make_unique<OpaqueRenderpass>();
    std::unique_ptr<TransparencyRenderpass> transparencyRenderpass = std::make_unique<TransparencyRenderpass>();
    std::unique_ptr<ResolverRenderpass> resolverRenderpass = std::make_unique<ResolverRenderpass>();
    std::unique_ptr<FXAARenderpass> fxaaRenderpass = std::make_unique<FXAARenderpass>();

    LightProcessor& lightProcessor = shadowpass->getLightProcessor();
    m_renderResources.priodLightsBuffer.reserve(lightProcessor.totalSupportedLights());
    m_currentRenderContext.lInfo.shadowMapAtlases = lightProcessor.getShadowMapAtlases();
    m_currentRenderContext.lInfo.shadowMapSizes = lightProcessor.getShadowMapSizes();
    m_currentRenderContext.lInfo.lightBuff = lightProcessor.getShaderLightUniformBuffer();
    m_currentRenderContext.lInfo.lightViewProjectionBuff = lightProcessor.getLightViewProjectionUniformBuffer();

    renderpasses.push_back(std::move(transformFeebackpass));
    renderpasses.push_back(std::move(shadowpass));
    renderpasses.push_back(std::move(ssaoRenderpass));
    renderpasses.push_back(std::move(opaqueRenderpass));
    renderpasses.push_back(std::move(transparencyRenderpass));
    renderpasses.push_back(std::move(resolverRenderpass));
    renderpasses.push_back(std::move(fxaaRenderpass));

    m_renderResources.lightsToRender = &m_lightsToRender;
    m_renderResources.objectsToRender = &m_objectsToRender;

    // Create vertex array / buffer for fullscreen quad
    m_fullScreenQuad_vbo = VertexBuffer::create(fullScreenQuadCW, sizeof(fullScreenQuadCW));
    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 2);  // Position
    layout.push(GL_FLOAT, 2);  // Screen UV
    m_fullScreenQuad_vbo.setLayout(layout);
    m_fullScreenQuad_vao = VertexArray::create();
    m_fullScreenQuad_vao.addBuffer(m_fullScreenQuad_vbo);

    FrameBuffer::bindDefault();
}

void Renderer::submitLight(Light* light) { m_lightsToRender.push_back(light); }

void Renderer::submitRenderable(Renderable* obj) {
    if (!obj->isReady()) return;
    m_objectsToRender.push_back(obj);
}

void Renderer::render(const ApplicationContext& context) {
    // Update render context
    glm::uvec2 newScreenRes = glm::uvec2(context.state.screenWidth, context.state.screenHeight);
    m_currentRenderContext.screenResChanged = newScreenRes != m_currentRenderContext.currScreenRes;
    m_currentRenderContext.currScreenRes = newScreenRes;
    m_currentRenderContext.deltaTime = context.instance->gameState.deltaTime;
    m_currentRenderContext.elapsedTime = context.instance->gameState.elapsedGameTime;

    m_renderResources.culledObjectsBuffer.reserve(m_objectsToRender.size());

    // Update camera aspect ratio just in case it changed via resize of screen.
    context.instance->m_player->getCamera()->setAspectRatio(
        static_cast<float>(context.state.screenWidth) / static_cast<float>(context.state.screenHeight)
    );

    auto start = std::chrono::high_resolution_clock::now();
    for (const std::unique_ptr<Renderpass>& pass : renderpasses) {
        pass->run(m_currentRenderContext, m_renderResources, context);
    }
    auto end = std::chrono::high_resolution_clock::now();

    m_lastLightCount = static_cast<int>(m_lightsToRender.size());
    m_lastObjectCount = static_cast<int>(m_objectsToRender.size());
    m_lastRenderTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    m_lightsToRender.clear();
    m_objectsToRender.clear();
}

void Renderer::drawFullscreenQuad() {
    m_fullScreenQuad_vao.bind();
    GLCALL(glDrawArrays(GL_TRIANGLES, 0, 6));
}

void Renderer::fillDebugReport(DebugReport& report) const {
    report.beginGroup("Renderer Stats");
    report.addTimeMs("Total processing time", m_lastRenderTimeMs);
    report.addCounter("Submitted objects", m_lastObjectCount);
    report.addCounter("Submitted lights", m_lastLightCount);
    for (const std::unique_ptr<Renderpass>& pass : renderpasses) {
        pass->putDebugInfo(report);
    }
    report.endGroup();
}
