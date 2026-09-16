#include "ShadowRenderpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "Logger.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Frustum.h"
#include "engine/scene/renderables/SkeletalMesh.h"
#include "game/GameInstance.h"

void ShadowRenderpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_lightProcessor.clearShadowMaps();
    m_lightProcessor.prepareShadowData(*resources.lightsToRender, resources.priodLightsBuffer, context);
    context.lighting.activeLightsCount = resources.priodLightsBuffer.size();

    // Depth offset to avoid shadow acne
    GLCALL(glEnable(GL_POLYGON_OFFSET_FILL));
    GLCALL(glPolygonOffset(2.0f, 1.0f));

    m_objectsProcessed = 0;
}

void ShadowRenderpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    filterForPass(*resources.objectsToRender, resources.passObjectsBuffer);

    for (const GPUShadowMap& shadowMap : m_lightProcessor.getAllocatedShadowMaps()) {
        const Light* light = resources.priodLightsBuffer[shadowMap.lightIndex];
        context.lighting.viewport.viewProjection = shadowMap.viewProjection;
        context.lighting.viewport.transform = light->getGlobalTransform();

        m_lightProcessor.prepareShadowPass(light, shadowMap);

        cullObjectsOutOfView(resources.passObjectsBuffer, resources.culledObjectsBuffer, shadowMap.viewProjection);
        batchForPass(resources.culledObjectsBuffer, context);

        renderBatches(context, resources);
        m_shaderBatches.clear();
    }
}

void ShadowRenderpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_processedLightCount = resources.priodLightsBuffer.size();
    m_shadowMapCount = m_lightProcessor.getAllocatedShadowMaps().size();

    GLCALL(glDisable(GL_POLYGON_OFFSET_FILL));
}

bool ShadowRenderpass::accepts(const Renderable* obj) const {
    const Material* material = obj->getMaterial().get(); 
    return material->surface == MaterialSurface::Opaque && material->castShadows;
}

ShaderKey ShadowRenderpass::makeShaderKey(const Renderable* obj, const RenderContext& context) const {
    const Material* material = obj->getMaterial().get();
    ShaderKey key{};
    key.pass = PassType::Shadow;
    key.geometry = obj->geometryType();
    key.features.baseColorTexture = material->baseColorTexture.isReady();
    key.features.alphaTest = material->alphaCutoff > 0.0f;
    return key;
}

ShadowRenderpass::ShadowRenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder)
    : Renderpass(shaderManager, binder), m_lightProcessor(MAX_LIGHTS) {}

const char* ShadowRenderpass::name() { return "Shadow Pass"; }

void ShadowRenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.addCounter("Lights processed", static_cast<int>(m_processedLightCount));
    report.addCounter("Shadowmaps generated", static_cast<int>(m_shadowMapCount));
    report.endGroup();
}
