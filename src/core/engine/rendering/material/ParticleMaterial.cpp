#include "ParticleMaterial.h"

#include "Logger.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/particles/ParticleSystem.h"

bool ParticleMaterial::isReady() const {
    return m_mainShader.isReady() && m_tfShader.isReady() && m_textureAtlas.isReady();
}

bool ParticleMaterial::supportsPass(PassType passType) const {
    return passType == PassType::TransformFeedback || passType == PassType::OpaquePass;
}

void ParticleMaterial::bindForPass(PassType passType, const RenderContext& context) {
    if (passType == PassType::TransformFeedback) {
        TransformFeedbackShader& tfShader = m_tfShader.value();

        tfShader.use();
        tfShader.setUniform("u_deltaTime", context.deltaTime);
        tfShader.setUniform("u_time", context.elapsedTime);
    } else if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_cameraRight", context.tInfo.viewportTransform.getRight());
        mainShader.setUniform("u_cameraUp", context.tInfo.viewportTransform.getUp());
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}

void ParticleMaterial::bindForObjectDraw(PassType passType, const RenderContext& context) {
    if (passType == PassType::TransformFeedback) {
        TransformFeedbackShader& tfShader = m_tfShader.value();

        tfShader.use();
        tfShader.bindUniformBuffer("ParticleModulesBlock", *context.pInfo.pModulesBuff);
        tfShader.setUniform(
            "u_moduleCount",
            static_cast<unsigned int>(context.pInfo.pModulesBuff->getByteSize() / sizeof(GenericGPUParticleModule))
        );
        tfShader.setUniform("u_spawnCount", context.pInfo.spawnCount);
        tfShader.setUniform("u_particleSpawnOffset", context.pInfo.particleSpawnOffset);
        tfShader.setUniform("u_allocatedParticleCount", context.pInfo.allocatedParticleCount);
        tfShader.setUniform("u_flags", context.pInfo.flags);
    } else if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_mvp", context.tInfo.viewProjection * context.tInfo.meshTransform.getModelMatrix());
        // Pass texture data
        bool useTexture = (context.pInfo.flags & USES_TEXTURE);
        mainShader.setUniform("u_useTexture", useTexture);
        if (useTexture && m_textureAtlas.isReady()) {
            Texture& texAtlas = m_textureAtlas.value();
            texAtlas.bindToUnit(0);
            mainShader.setUniform("u_textureAtlas", 0);
            mainShader.setUniform("u_textureAtlasSize", texAtlas.width());
            mainShader.setUniform("u_textureSize", 16u);
        }
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}