#ifndef TOOMANYBLOCKS_CHUNKMATERIAL_H
#define TOOMANYBLOCKS_CHUNKMATERIAL_H

#include "engine/rendering/material/Material.h"
#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/opengl/Texture.h"
#include "foundation/threading/Future.h"

class ChunkMaterial : public Material {
private:
    Future<Shader> m_mainShader;
    Future<Shader> m_depthShader;
    Future<Shader> m_ssaoGBuffShader;
    Future<Texture> m_textureAtlas;

public:
    ChunkMaterial(
        Future<Shader> mainShader,
        Future<Shader> depthShader,
        Future<Shader> ssaoGBuffShader,
        Future<Texture> textureAtlas
    )
        : m_mainShader(mainShader),
          m_textureAtlas(textureAtlas),
          m_ssaoGBuffShader(ssaoGBuffShader),
          m_depthShader(depthShader) {}

    virtual ~ChunkMaterial() = default;

    bool isReady() const override;

    bool supportsPass(PassType passType) const override;

    void bindForPass(PassType passType, const RenderContext& context) override;

    void bindForObjectDraw(PassType passType, const RenderContext& context) override;
};

#endif