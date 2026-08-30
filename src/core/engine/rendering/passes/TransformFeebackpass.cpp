#include "TransformFeebackpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Camera.h"
#include "engine/rendering/camera/Frustum.h"
#include "engine/rendering/particles/ParticleSystem.h"
#include "game/GameInstance.h"

void TransformFeedbackpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glEnable(GL_RASTERIZER_DISCARD));
    context.tInfo.viewProjection = appContext.instance->m_player->getCamera()->getViewProjMatrix();
    context.tInfo.viewportTransform = appContext.instance->m_player->getCamera()->getGlobalTransform();
}

void TransformFeedbackpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    cullObjectsOutOfView(*resources.objectsToRender, resources.culledObjectsBuffer, context.tInfo.viewProjection);
    batchByMaterialForPass(resources.culledObjectsBuffer, PassType::TransformFeedback);

    m_objectsProcessed = 0;
    for (auto& batch : m_materialBatches) {
        batch.first->bindForPass(PassType::TransformFeedback, context);

        for (Renderable* obj : batch.second) {
            if (ParticleSystem* ps = dynamic_cast<ParticleSystem*>(obj)) {
                ps->switchBuffers();
                context.tInfo.meshTransform = ps->getRenderableTransform();
                context.pInfo.pModulesBuff = ps->getModulesUBO();
                context.pInfo.spawnCount = ps->getSpawnCount();
                context.pInfo.particleSpawnOffset = ps->getParticleSpawnOffset();
                context.pInfo.allocatedParticleCount = ps->getAllocatedParticleCount();
                context.pInfo.flags = ps->getFlags();
                batch.first->bindForObjectDraw(PassType::TransformFeedback, context);
                ps->compute();
            }
            m_objectsProcessed++;
        }
    }

    m_materialBatches.clear();
}

void TransformFeedbackpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glDisable(GL_RASTERIZER_DISCARD));
}

const char* TransformFeedbackpass::name() { return "Transform Feedback Pass"; }

void TransformFeedbackpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.endGroup();
}
