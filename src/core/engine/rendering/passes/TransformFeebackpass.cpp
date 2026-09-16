#include "TransformFeebackpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Frustum.h"
#include "engine/rendering/particles/ParticleSystem.h"
#include "game/GameInstance.h"

void TransformFeedbackpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glEnable(GL_RASTERIZER_DISCARD));
    m_objectsProcessed = 0;
}

void TransformFeedbackpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    filterForPass(*resources.objectsToRender, resources.passObjectsBuffer);
    batchForPass(resources.passObjectsBuffer, context);
    renderBatches(context, resources);
}

void TransformFeedbackpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_shaderBatches.clear();

    GLCALL(glDisable(GL_RASTERIZER_DISCARD));
}

bool TransformFeedbackpass::accepts(const Renderable* obj) const {
    return dynamic_cast<const TransformFeedbackRenderable*>(obj);
}

ShaderKey TransformFeedbackpass::makeShaderKey(const Renderable* obj, const RenderContext& context) const {
    const Material* material = obj->getMaterial().get();
    ShaderKey key{};
    key.pass = PassType::TransformFeedback;
    key.geometry = obj->geometryType();
    return key;
}

void TransformFeedbackpass::drawRenderable(Renderable* obj) {
    if (TransformFeedbackRenderable* tfObj = static_cast<TransformFeedbackRenderable*>(obj)) {
        tfObj->switchBuffers();
        tfObj->compute();
    }
}

TransformFeedbackpass::TransformFeedbackpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder)
    : Renderpass(shaderManager, binder) {}

const char* TransformFeedbackpass::name() { return "Transform Feedback Pass"; }

void TransformFeedbackpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.endGroup();
}
