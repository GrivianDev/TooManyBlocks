#ifndef TOOMANYBLOCKS_SIMPLEMATERIAL_H
#define TOOMANYBLOCKS_SIMPLEMATERIAL_H

#include <glm/vec3.hpp>

#include "engine/rendering/lowlevelapi/Shader.h"
#include "engine/rendering/lowlevelapi/Texture.h"
#include "engine/rendering/mat/Material.h"
#include "foundation/threading/Future.h"

class SimpleMaterial : public Material {
private:
    Future<Shader> m_mainShader;
    Future<Shader> m_depthShader;
    glm::vec3 m_color;
    Future<Texture> m_texture;

public:
    SimpleMaterial(Future<Shader> mainShader, Future<Shader> depthShader, const glm::vec3& color, Future<Texture> texture = Future<Texture>())
        : m_mainShader(mainShader), m_depthShader(depthShader), m_color(color), m_texture(texture) {}

    virtual ~SimpleMaterial() = default;

    bool isReady() const override;

    bool supportsPass(PassType passType) const override;

    void bindForPass(PassType passType, const RenderContext& context) override;

    void bindForObjectDraw(PassType passType, const RenderContext& context) override;
};

#endif