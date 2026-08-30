#ifndef TOOMANYBLOCKS_SKELETALMATERIAL_H
#define TOOMANYBLOCKS_SKELETALMATERIAL_H

#include "engine/rendering/material/Material.h"
#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/opengl/Texture.h"
#include "foundation/threading/Future.h"

class SkeletalMaterial : public Material {
private:
    Future<Shader> m_mainShader;
    Future<Shader> m_depthShader;
    Future<Texture> m_texture;

public:
    SkeletalMaterial(Future<Shader> mainShader, Future<Shader> depthShader, Future<Texture> texture)
        : m_mainShader(mainShader), m_depthShader(depthShader), m_texture(texture) {}

    virtual ~SkeletalMaterial() = default;

    bool isReady() const override;

    bool supportsPass(PassType passType) const override;

    void bindForPass(PassType passType, const RenderContext& context) override;

    void bindForObjectDraw(PassType passType, const RenderContext& context) override;
};

#endif