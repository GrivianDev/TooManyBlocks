#include "Renderer.h"

#include <GL/glew.h>

#include <chrono>

#include "Application.h"
#include "Logger.h"
#include "engine/GameInstance.h"
#include "engine/rendering/Camera.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/renderpasses/FXAARenderpass.h"
#include "engine/rendering/renderpasses/FinalOutputpass.h"
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

void Renderer::queryGraphicsInfo() {
    // Basic identification
    const GLubyte* vendor = nullptr;
    GLCALL(vendor = glGetString(GL_VENDOR));
    if (vendor) {
        m_graphicsInfo.vendor = reinterpret_cast<const char*>(vendor);
    }

    const GLubyte* renderer = nullptr;
    GLCALL(renderer = glGetString(GL_RENDERER));
    if (renderer) {
        m_graphicsInfo.renderer = reinterpret_cast<const char*>(renderer);
    }

    const GLubyte* version = nullptr;
    GLCALL(version = glGetString(GL_VERSION));
    if (version) {
        m_graphicsInfo.version = reinterpret_cast<const char*>(version);
    }

    const GLubyte* shadingLanguageVersion = nullptr;
    GLCALL(shadingLanguageVersion = glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (shadingLanguageVersion) {
        m_graphicsInfo.shadingLanguageVersion = reinterpret_cast<const char*>(shadingLanguageVersion);
    }

    // OpenGL context
    GLCALL(glGetIntegerv(GL_MAJOR_VERSION, &m_graphicsInfo.context.majorVersion));
    GLCALL(glGetIntegerv(GL_MINOR_VERSION, &m_graphicsInfo.context.minorVersion));
    GLint profile = 0;
    GLCALL(glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile));
    m_graphicsInfo.context.coreProfile = (profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
    m_graphicsInfo.context.compatibilityProfile = (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
    GLint flags = 0;
    GLCALL(glGetIntegerv(GL_CONTEXT_FLAGS, &flags));
    m_graphicsInfo.context.debugContext = (flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
    m_graphicsInfo.context.forwardCompatible = (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0;

#ifdef GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT
    m_graphicsInfo.context.robustAccess = (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT) != 0;
#endif

    // Limits
    GLCALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_graphicsInfo.limits.maxTextureSize));
    GLCALL(glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_graphicsInfo.limits.max3DTextureSize));
    GLCALL(glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &m_graphicsInfo.limits.maxCubeMapTextureSize));
    GLCALL(glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_graphicsInfo.limits.maxArrayTextureLayers));
    GLCALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_graphicsInfo.limits.maxTextureImageUnits));
    GLCALL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_graphicsInfo.limits.maxCombinedTextureImageUnits));
    GLCALL(glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &m_graphicsInfo.limits.maxVertexTextureImageUnits));
    GLCALL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_graphicsInfo.limits.maxVertexAttribs));
    GLCALL(glGetIntegerv(GL_MAX_DRAW_BUFFERS, &m_graphicsInfo.limits.maxDrawBuffers));
    GLCALL(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_graphicsInfo.limits.maxColorAttachments));
    GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &m_graphicsInfo.limits.maxSamples));

    // Buffer limits
    GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &m_graphicsInfo.limits.maxUniformBufferBindings));
    GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &m_graphicsInfo.limits.maxUniformBlockSize));

#ifdef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
    GLCALL(glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &m_graphicsInfo.limits.maxShaderStorageBufferBindings));
#endif

#ifdef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
    GLint64 maxSSBOSize = 0;

    GLCALL(glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize));

    m_graphicsInfo.limits.maxShaderStorageBlockSize = static_cast<int>(
        std::min<GLint64>(maxSSBOSize, std::numeric_limits<int>::max())
    );
#endif

    // Shader limits
    GLCALL(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &m_graphicsInfo.limits.maxVertexUniformComponents));
    GLCALL(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &m_graphicsInfo.limits.maxFragmentUniformComponents));

#ifdef GL_MAX_VARYING_VECTORS
    GLCALL(glGetIntegerv(GL_MAX_VARYING_VECTORS, &m_graphicsInfo.limits.maxVaryingVectors));
#endif

    // Compute shader limits
#ifdef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
    GLCALL(glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &m_graphicsInfo.limits.maxComputeWorkGroupInvocations));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &m_graphicsInfo.limits.maxComputeWorkGroupCount[0]));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &m_graphicsInfo.limits.maxComputeWorkGroupCount[1]));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &m_graphicsInfo.limits.maxComputeWorkGroupCount[2]));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_graphicsInfo.limits.maxComputeWorkGroupSize[0]));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &m_graphicsInfo.limits.maxComputeWorkGroupSize[1]));
    GLCALL(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &m_graphicsInfo.limits.maxComputeWorkGroupSize[2]));
#endif

    // Viewport
    GLint viewport[2] = {};
    GLCALL(glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewport));
    m_graphicsInfo.limits.maxViewportWidth = viewport[0];
    m_graphicsInfo.limits.maxViewportHeight = viewport[1];
}

void Renderer::init() {
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Error initializing glew!");
    }

    // GLEnableDebugging();

    queryGraphicsInfo();

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
    std::unique_ptr<FinalOutputpass> outputPass = std::make_unique<FinalOutputpass>();

    LightProcessor& lightProcessor = shadowpass->getLightProcessor();
    m_renderResources.priodLightsBuffer.reserve(lightProcessor.getTotalSupportedLights());
    m_currentRenderContext.lInfo.shadowMapAtlases = lightProcessor.getShadowMapAtlases();
    m_currentRenderContext.lInfo.lightBuff = lightProcessor.getLightUniformBuffer();
    m_currentRenderContext.lInfo.shadowMapBuff = lightProcessor.getShadowMapUniformBuffer();

    m_renderpasses.push_back(std::move(transformFeebackpass));
    m_renderpasses.push_back(std::move(shadowpass));
    m_renderpasses.push_back(std::move(ssaoRenderpass));
    m_renderpasses.push_back(std::move(opaqueRenderpass));
    m_renderpasses.push_back(std::move(transparencyRenderpass));
    m_renderpasses.push_back(std::move(resolverRenderpass));
    m_renderpasses.push_back(std::move(fxaaRenderpass));
    m_renderpasses.push_back(std::move(outputPass));

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
    if (!obj->isReady() || !obj->isVisible()) return;
    m_objectsToRender.push_back(obj);
}

void Renderer::render(const ApplicationContext& context) {
    // Update render context
    glm::uvec2 newScreenRes = glm::uvec2(context.state.screenWidth, context.state.screenHeight);
    m_currentRenderContext.screenResChanged = newScreenRes != m_currentRenderContext.currScreenRes;
    m_currentRenderContext.currScreenRes = newScreenRes;
    m_currentRenderContext.deltaTime = context.instance->gameState.deltaTime;
    m_currentRenderContext.elapsedTime = context.instance->gameState.elapsedGameTime;

    m_currentRenderContext.ssaoInfo.enabled = getPass<SSAORenderpass>()->isEnabled();
    m_currentRenderContext.fxaaInfo.enabled = getPass<FXAARenderpass>()->isEnabled();

    m_renderResources.culledObjectsBuffer.reserve(m_objectsToRender.size());

    // Update camera aspect ratio just in case it changed via resize of screen.
    context.instance->m_player->getCamera()->setAspectRatio(
        static_cast<float>(context.state.screenWidth) / static_cast<float>(context.state.screenHeight)
    );

    auto start = std::chrono::high_resolution_clock::now();
    for (const std::unique_ptr<Renderpass>& pass : m_renderpasses) {
        if (!pass->isEnabled()) continue;
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
    for (const std::unique_ptr<Renderpass>& pass : m_renderpasses) {
        pass->putDebugInfo(report);
    }
    report.endGroup();
}

void Renderer::setDebugPolygonModeEnabled(bool enabled) {
    for (const std::unique_ptr<Renderpass>& pass : m_renderpasses) {
        if (OpaqueRenderpass* passPtr = dynamic_cast<OpaqueRenderpass*>(pass.get())) {
            passPtr->setDebugPolygonModeEnabled(enabled);
        }
    }
}
