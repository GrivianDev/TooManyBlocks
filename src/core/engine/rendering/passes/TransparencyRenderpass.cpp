#include "TransparencyRenderpass.h"

#include <GL/glew.h>

#include "Application.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Camera.h"
#include "engine/rendering/camera/Frustum.h"
#include "game/GameInstance.h"

static constexpr float zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
static constexpr float one[] = {1.0f, 1.0f, 1.0f, 1.0f};

void TransparencyRenderpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    if (context.screenResChanged) {
        createBuffers(context);
    }

    m_accAndResBuffer.bind();
    GLCALL(glClearBufferfv(GL_COLOR, 0, zero));
    GLCALL(glClearBufferfv(GL_COLOR, 1, one));  // Reveal target must start at 1.0f

    GLCALL(glEnable(GL_BLEND));

    GLCALL(glBlendEquationi(0, GL_FUNC_ADD));
    GLCALL(glBlendFunci(0, GL_ONE, GL_ONE));
    GLCALL(glBlendEquationi(1, GL_FUNC_ADD));
    GLCALL(glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR));

    // Disable depth write, transparent pixels do not cover objects
    GLCALL(glDepthMask(GL_FALSE));

    context.tInfo.viewProjection = appContext.instance->m_player->getCamera()->getViewProjMatrix();
    context.tInfo.projection = appContext.instance->m_player->getCamera()->getProjectionMatrix();
    context.tInfo.view = appContext.instance->m_player->getCamera()->getViewMatrix();
    context.tInfo.viewportTransform = appContext.instance->m_player->getCamera()->getGlobalTransform();
}

void TransparencyRenderpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_objectsProcessed = 0;

    cullObjectsOutOfView(*resources.objectsToRender, resources.culledObjectsBuffer, context.tInfo.viewProjection);
    batchByMaterialForPass(resources.culledObjectsBuffer, PassType::TransparencyPass);

    for (const auto& batch : m_materialBatches) {
        batch.first->bindForPass(PassType::TransparencyPass, context);

        for (const Renderable* obj : batch.second) {
            context.tInfo.meshTransform = obj->getRenderableTransform();
            batch.first->bindForObjectDraw(PassType::TransparencyPass, context);
            obj->draw();

            m_objectsProcessed++;
        }
    }

    m_materialBatches.clear();
}

void TransparencyRenderpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glDisable(GL_BLEND));
    GLCALL(glDepthMask(GL_TRUE));

    context.transparencyInfo.accumOutput = m_accAndResBuffer.getAttachedTextures().at(0).get();
    context.transparencyInfo.revealOutput = m_accAndResBuffer.getAttachedTextures().at(1).get();
}

TransparencyRenderpass::TransparencyRenderpass() { m_accAndResBuffer = FrameBuffer::create(); }

const char* TransparencyRenderpass::name() { return "Transparency Renderpass"; }

void TransparencyRenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.endGroup();
}

void TransparencyRenderpass::createBuffers(RenderContext& context) {
    m_accAndResBuffer.clearAttachedTextures();
    m_accAndResBuffer.attachTexture(
        std::make_shared<Texture>(  // Color accumulate render target
            Texture::create(TextureType::Float16, context.currScreenRes.x, context.currScreenRes.y, 4)
        )
    );
    m_accAndResBuffer.attachTexture(
        std::make_shared<Texture>(  // Reveal alpha render target
            Texture::create(TextureType::Float16, context.currScreenRes.x, context.currScreenRes.y, 1)
        )
    );
    // Use same depth buffer as opaque pass !!! OPAQUE PASS MUST RESIZE !!!
    m_accAndResBuffer.attachTexture(context.opaqueInfo.usedDepthTexture);
}
