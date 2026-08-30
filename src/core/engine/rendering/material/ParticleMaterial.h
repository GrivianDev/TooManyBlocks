#ifndef TOOMANYBLOCKS_PARTICLEMATERIAL_H
#define TOOMANYBLOCKS_PARTICLEMATERIAL_H

#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/opengl/TransformFeedbackShader.h"
#include "engine/rendering/opengl/Texture.h"
#include "engine/rendering/material/Material.h"
#include "foundation/threading/Future.h"

class ParticleMaterial : public Material {
private:
    Future<Shader> m_mainShader;
    Future<TransformFeedbackShader> m_tfShader;
    Future<Texture> m_textureAtlas;

public:
    ParticleMaterial(
        Future<Shader> mainShader,
        Future<TransformFeedbackShader> tfShader,
        Future<Texture> textureAtlas = Future<Texture>()
    )
        : m_mainShader(mainShader), m_tfShader(tfShader), m_textureAtlas(textureAtlas) {}
    virtual ~ParticleMaterial() = default;

    bool isReady() const override;

    bool supportsPass(PassType passType) const override;

    void bindForPass(PassType passType, const RenderContext& context) override;

    void bindForObjectDraw(PassType passType, const RenderContext& context) override;
};

#endif