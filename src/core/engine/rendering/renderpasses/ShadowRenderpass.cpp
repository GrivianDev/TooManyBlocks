#include "ShadowRenderpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "Logger.h"
#include "datatypes/Transform.h"
#include "engine/GameInstance.h"
#include "engine/rendering/Camera.h"
#include "engine/rendering/Frustum.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"

void ShadowRenderpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_lightProcessor.clearShadowMaps();

    context.tInfo.view = appContext.instance->m_player->getCamera()->getViewMatrix();
    context.tInfo.projection = appContext.instance->m_player->getCamera()->getProjectionMatrix();
    context.tInfo.viewportTransform = appContext.instance->m_player->getCamera()->getGlobalTransform();
    context.tInfo.viewProjection = appContext.instance->m_player->getCamera()->getViewProjMatrix();
    m_lightProcessor.prepareShadowData(*resources.lightsToRender, resources.priodLightsBuffer, context);
    context.lInfo.activeLightsCount = resources.priodLightsBuffer.size();

    // Depth offset to avoid shadow acne
    GLCALL(glEnable(GL_POLYGON_OFFSET_FILL));
    GLCALL(glPolygonOffset(1.0f, 1.0f));
}

void ShadowRenderpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_objectsProcessed = 0;
    m_processedLightCount = 0;
    m_shadowMapCount = 0;

    for (const GPUShadowMap& shadowMap : m_lightProcessor.getAllocatedShadowMaps()) {
        const Light* light = resources.priodLightsBuffer[shadowMap.lightIndex];
        context.tInfo.viewProjection = shadowMap.viewProjection;
        context.tInfo.viewportTransform = light->getGlobalTransform();

        m_lightProcessor.prepareShadowPass(light, shadowMap);

        cullObjectsOutOfView(*resources.objectsToRender, resources.culledObjectsBuffer, context.tInfo.viewProjection);
        batchByMaterialForPass(resources.culledObjectsBuffer, PassType::ShadowPass);

        for (const auto& batch : m_materialBatches) {
            batch.first->bindForPass(PassType::ShadowPass, context);

            for (const Renderable* obj : batch.second) {
                context.tInfo.meshTransform = obj->getRenderableTransform();
                batch.first->bindForObjectDraw(PassType::ShadowPass, context);
                obj->draw();

                m_objectsProcessed++;
            }
        }
        m_materialBatches.clear();
    }
    m_processedLightCount = resources.priodLightsBuffer.size();
    m_shadowMapCount = m_lightProcessor.getAllocatedShadowMaps().size();
}

void ShadowRenderpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glDisable(GL_POLYGON_OFFSET_FILL));
}

ShadowRenderpass::ShadowRenderpass() : m_lightProcessor(MAX_LIGHTS) {}

const char* ShadowRenderpass::name() { return "Shadow Pass"; }

void ShadowRenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.addCounter("Lights processed", static_cast<int>(m_processedLightCount));
    report.addCounter("Shadowmaps generated", static_cast<int>(m_shadowMapCount));
    report.endGroup();
}
