#ifndef TOOMANYBLOCKS_TRANSPARENTMATERIAL_H
#define TOOMANYBLOCKS_TRANSPARENTMATERIAL_H

#include <glm/vec4.hpp>

#include "engine/rendering/material/Material.h"
#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/opengl/Texture.h"
#include "foundation/threading/Future.h"

class TransparentMaterial : public Material {
private:
    Future<Shader> m_mainShader;
    glm::vec4 m_color;
    Future<Texture> m_texture;

public:
    TransparentMaterial(Future<Shader> mainShader, const glm::vec4 color, Future<Texture> texture = Future<Texture>())
        : m_mainShader(mainShader), m_color(color), m_texture(texture) {}

    virtual ~TransparentMaterial() = default;

    bool isReady() const override;

    bool supportsPass(PassType passType) const override;

    void bindForPass(PassType passType, const RenderContext& context) override;

    void bindForObjectDraw(PassType passType, const RenderContext& context) override;
};

#endif
