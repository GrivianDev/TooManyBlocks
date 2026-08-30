#include "LineMaterial.h"

#include "Logger.h"
#include "engine/rendering/Renderer.h"

bool LineMaterial::isReady() const { return m_mainShader.isReady(); }

bool LineMaterial::supportsPass(PassType passType) const { return passType == PassType::OpaquePass; }

void LineMaterial::bindForPass(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_color", m_color);
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}

void LineMaterial::bindForObjectDraw(PassType passType, const RenderContext& context) {
    if (passType == PassType::OpaquePass) {
        Shader& mainShader = m_mainShader.value();

        mainShader.use();
        mainShader.setUniform("u_mvp", context.tInfo.viewProjection * context.tInfo.meshTransform.getModelMatrix());
    } else {
        lgr::lout.error("Material bound for unsupported pass");
    }
}