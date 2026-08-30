#include "TransparentMaterial.h"

#include "Logger.h"
#include "engine/rendering/Renderer.h"

bool TransparentMaterial::isReady() const { return m_mainShader.isReady(); }

bool TransparentMaterial::supportsPass(PassType passType) const { return passType == PassType::TransparencyPass; }

void TransparentMaterial::bindForPass(PassType passType, const RenderContext& context) {
    if (passType == PassType::TransparencyPass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_color", m_color);
        mainShader.setUniform("u_useTexture", m_texture.isReady());
        if (m_texture.isReady()) {
            m_texture.value().bindToUnit(0);
            mainShader.setUniform("u_texture", 0);
        }
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}

void TransparentMaterial::bindForObjectDraw(PassType passType, const RenderContext& context) {
    if (passType == PassType::TransparencyPass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_mvp", context.tInfo.viewProjection * context.tInfo.meshTransform.getModelMatrix());
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}