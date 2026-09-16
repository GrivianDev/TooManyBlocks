#include "SSAORenderpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Frustum.h"
#include "game/GameInstance.h"

bool SSAORenderpass::accepts(const Renderable* obj) const {
    const Material* material = obj->getMaterial().get();
    return material->surface == MaterialSurface::Opaque && material->occludes;
}

ShaderKey SSAORenderpass::makeShaderKey(const Renderable* obj, const RenderContext& context) const {
    const Material* material = obj->getMaterial().get();
    ShaderKey key{};
    key.pass = PassType::AmbientOcclusion;
    key.geometry = obj->geometryType();
    key.features.baseColorTexture = material->baseColorTexture.isReady();
    key.features.alphaTest = material->alphaCutoff > 0.0f;
    return key;
}

void SSAORenderpass::prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    m_ssaoProcessor.validateBuffers(appContext);  // Possible resize ssao textures if resize happened
    m_ssaoProcessor.prepareSSAOGBufferPass(appContext);
    m_objectsProcessed = 0;
}

void SSAORenderpass::execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    filterForPass(*resources.objectsToRender, resources.passObjectsBuffer);
    cullObjectsOutOfView(resources.passObjectsBuffer, resources.culledObjectsBuffer, context.viewport.viewProjection);
    batchForPass(resources.culledObjectsBuffer, context);
    renderBatches(context, resources);

    m_ssaoProcessor.prepareSSAOPass(appContext);
    appContext.renderer->drawFullscreenQuad();

    m_ssaoProcessor.prepareSSAOBlurPass(appContext);
    appContext.renderer->drawFullscreenQuad();
}

void SSAORenderpass::cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    m_shaderBatches.clear();
    context.ssao.output = m_ssaoProcessor.getOcclusionOutput();
}

SSAORenderpass::SSAORenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder)
    : Renderpass(shaderManager, binder) {}

const char* SSAORenderpass::name() { return "SSAO Renderpass"; }

void SSAORenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.endGroup();
}
