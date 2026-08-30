#include "SimpleMaterial.h"

#include "Logger.h"
#include "engine/rendering/Renderer.h"

bool SimpleMaterial::isReady() const { return m_mainShader.isReady() && m_depthShader.isReady(); }

bool SimpleMaterial::supportsPass(PassType passType) const {
    return passType == PassType::ShadowPass || passType == PassType::OpaquePass;
}

void SimpleMaterial::bindForPass(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_view", context.tInfo.view);
        mainShader.setUniform("u_cameraPosition", context.tInfo.viewportTransform.getPosition());
        mainShader.setUniform("u_color", m_color);
        mainShader.setUniform("u_useTexture", !m_texture.isEmpty());
        if (m_texture.isReady()) {
            m_texture.value().bindToUnit(0);
            mainShader.setUniform("u_texture", 0);
        }

        // Pass light info
        mainShader.setUniform("u_lightCount", static_cast<int>(context.lInfo.activeLightsCount));
        mainShader.bindUniformBuffer("LightsBlock", *context.lInfo.lightBuff);
        mainShader.bindUniformBuffer("ShadowMapsBlock", *context.lInfo.shadowMapBuff);

        // Pass depth buffers for shadowmapping
        for (int atlasIndex = 0; atlasIndex < SHADOW_ATLAS_COUNT; atlasIndex++) {
            if (const Texture* shadowAtlas = context.lInfo.shadowMapAtlases[atlasIndex]) {
                const std::string idxStr = std::to_string(atlasIndex);
                shadowAtlas->bindToUnit(atlasIndex + 2);
                mainShader.setUniform("u_shadowMapAtlas[" + idxStr + "]", atlasIndex + 2);
            } else {
                lgr::lout.error("Shadow map atlas for prio " + std::to_string(atlasIndex) + " not loaded");
            }
        }
    } else if (passType == PassType::ShadowPass) {
        // Nothing important to do here
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}

void SimpleMaterial::bindForObjectDraw(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_mvp", context.tInfo.viewProjection * context.tInfo.meshTransform.getModelMatrix());
        mainShader.setUniform("u_model", context.tInfo.meshTransform.getModelMatrix());
    } else if (passType == PassType::ShadowPass) {
        Shader& depthShader = m_depthShader.value();

        depthShader.use();
        depthShader.setUniform("u_mvp", context.tInfo.viewProjection * context.tInfo.meshTransform.getModelMatrix());
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}
