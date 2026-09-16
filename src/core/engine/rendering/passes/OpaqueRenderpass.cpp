#include "OpaqueRenderpass.h"

#include <GL/glew.h>

#include <glm/vec2.hpp>

#include "Application.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/camera/Frustum.h"
#include "engine/rendering/particles/ParticleSystem.h"
#include "engine/scene/renderables/SkeletalMesh.h"
#include "game/GameInstance.h"

void OpaqueRenderpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    if (context.screenResChanged) {
        createBuffers(context);
    }

    m_opaqueBuffer.bind();
    GLCALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    glm::uvec2 screenRes = context.currScreenRes;
    GLCALL(glViewport(0, 0, screenRes.x, screenRes.y));

    if (m_debugPolygonModeEnabled) {
        GLCALL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    }
    m_objectsProcessed = 0;
}

void OpaqueRenderpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    filterForPass(*resources.objectsToRender, resources.passObjectsBuffer);
    cullObjectsOutOfView(resources.passObjectsBuffer, resources.culledObjectsBuffer, context.viewport.viewProjection);
    batchForPass(resources.culledObjectsBuffer, context);
    renderBatches(context, resources);
}

void OpaqueRenderpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    m_shaderBatches.clear();

    context.opaque.output = m_opaqueBuffer.getAttachedTextures().at(0).get();

    if (m_debugPolygonModeEnabled) {
        GLCALL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    }
}

bool OpaqueRenderpass::accepts(const Renderable* obj) const {
    return obj->getMaterial()->surface == MaterialSurface::Opaque;
}

ShaderKey OpaqueRenderpass::makeShaderKey(const Renderable* obj, const RenderContext& context) const {
    const Material* material = obj->getMaterial().get();
    ShaderKey key{};
    key.pass = PassType::Opaque;
    key.geometry = obj->geometryType();
    key.features.lit = material->lit;
    key.features.baseColorTexture = material->baseColorTexture.isReady();
    key.features.alphaTest = material->alphaCutoff > 0.0f;
    key.features.ssao = material->occludes && context.ssao.enabled;
    return key;
}

OpaqueRenderpass::OpaqueRenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder)
    : Renderpass(shaderManager, binder), m_debugPolygonModeEnabled(false) {
    m_opaqueBuffer = FrameBuffer::create();
}

const char* OpaqueRenderpass::name() { return "Opaque Renderpass"; }

void OpaqueRenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.addCounter("Objects processed", static_cast<int>(m_objectsProcessed));
    report.endGroup();
}

void OpaqueRenderpass::createBuffers(RenderContext& context) {
    m_opaqueBuffer.clearAttachedTextures();
    m_opaqueBuffer.attachTexture(
        std::make_shared<Texture>(
            Texture::create(TextureType::Color, context.currScreenRes.x, context.currScreenRes.y, 3)
        )
    );
    m_opaqueBuffer.attachTexture(
        std::make_shared<Texture>(Texture::create(TextureType::Depth, context.currScreenRes.x, context.currScreenRes.y))
    );
    context.opaque.usedDepthTexture = m_opaqueBuffer.getAttachedDepthTexture();
}
