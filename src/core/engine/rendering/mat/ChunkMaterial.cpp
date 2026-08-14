#include "ChunkMaterial.h"

#include <GL/glew.h>

#include <sstream>

#include "Logger.h"
#include "engine/env/lights/Spotlight.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"

bool ChunkMaterial::isReady() const {
    return m_mainShader.isReady() && m_depthShader.isReady() && m_ssaoGBuffShader.isReady() && m_textureAtlas.isReady();
}

bool ChunkMaterial::supportsPass(PassType passType) const {
    return passType == PassType::ShadowPass || passType == PassType::AmbientOcclusion || passType == PassType::OpaquePass;
}

void ChunkMaterial::bindForPass(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_view", context.tInfo.view);
        mainShader.setUniform("u_viewProjection", context.tInfo.viewProjection);
        mainShader.setUniform("u_cameraPosition", context.tInfo.viewportTransform.getPosition());

        // Pass texture data
        if (m_textureAtlas.isReady()) {
            m_textureAtlas.value().bindToUnit(0);
            mainShader.setUniform("u_textureAtlas", 0);
            mainShader.setUniform("u_textureAtlasSize", m_textureAtlas.value().width());
            mainShader.setUniform("u_textureSize", 16u);
        } else {
            lgr::lout.error("Texture atlas not loaded for ChunkMaterial");
        }

        // Pass light info
        mainShader.setUniform("u_lightCount", static_cast<int>(context.lInfo.activeLightsCount));
        mainShader.bindUniformBuffer("LightsBlock", *context.lInfo.lightBuff);
        mainShader.bindUniformBuffer("ShadowMapsBlock", *context.lInfo.shadowMapBuff);

        mainShader.setUniform("u_ssaoEnabled", context.ssaoInfo.enabled);
        if (context.ssaoInfo.enabled) {
            context.ssaoInfo.output->bindToUnit(1);
            mainShader.setUniform("u_ssaoTexture", 1);
        }
        mainShader.setUniform("u_screenResolution", context.currScreenRes);

        // Pass depth buffers for shadowmapping
        for (int atlasIndex = 0; atlasIndex < SHADOW_ATLAS_COUNT; atlasIndex++) {
            if (const Texture* shadowAtlas = context.lInfo.shadowMapAtlases[atlasIndex]) {
                const std::string idxStr = std::to_string(atlasIndex);
                shadowAtlas->bindToUnit(atlasIndex + 2);
                mainShader.setUniform("u_shadowMapAtlas[" + idxStr + "]", atlasIndex + 2);
            } else {
                lgr::lout.error("Shadow map atlas for prio " + std::to_string(atlasIndex) + " not loaded for ChunkMaterial");
            }
        }
    } else if (passType == PassType::ShadowPass) {
        Shader& depthShader = m_depthShader.value();

        depthShader.use();
        depthShader.setUniform("u_viewProjection", context.tInfo.viewProjection);
    } else if (passType == PassType::AmbientOcclusion) {
        Shader& ssaoGBuffShader = m_ssaoGBuffShader.value();
        
        ssaoGBuffShader.use();
        ssaoGBuffShader.setUniform("u_view", context.tInfo.view);
        ssaoGBuffShader.setUniform("u_projection", context.tInfo.projection);
    }
}

void ChunkMaterial::bindForObjectDraw(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_chunkPosition", context.tInfo.meshTransform.getPosition());
    } else if (passType == PassType::ShadowPass) {
        Shader& depthShader = m_depthShader.value();

        depthShader.use();
        depthShader.setUniform("u_chunkPosition", context.tInfo.meshTransform.getPosition());
    } else if (passType == PassType::AmbientOcclusion) {
        Shader& ssaoGBuffShader = m_ssaoGBuffShader.value();

        ssaoGBuffShader.use();
        ssaoGBuffShader.setUniform("u_chunkPosition", context.tInfo.meshTransform.getPosition());
    }
}
