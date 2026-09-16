#ifndef TOOMANYBLOCKS_MATERIALSYSTEM_H
#define TOOMANYBLOCKS_MATERIALSYSTEM_H

#include "engine/rendering/opengl/Texture.h"
#include "foundation/threading/Future.h"

#include <glm/vec4.hpp>

enum class MaterialSurface {
    Opaque,
    Transparent
};

struct Material {
    MaterialSurface surface = MaterialSurface::Opaque;

    glm::vec4 baseColor = glm::vec4(0.0f);
    Future<Texture> baseColorTexture;

    float alphaCutoff = 0.0f;

    float specularFactor = 1.0f;
    float specularExponent = 30.0f;

    bool lit = true;
    bool castShadows = true;
    // Objects that produce in gbuffer output for occlusion MUST also consume the total occlusion when opaque rendering
    bool occludes = false;
};

#endif
