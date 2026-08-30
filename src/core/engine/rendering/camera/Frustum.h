#ifndef TOOMANYBLOCKS_FRUSTUM_H
#define TOOMANYBLOCKS_FRUSTUM_H

#include <glm/glm.hpp>
#include <vector>

#include "engine/scene/renderables/Renderable.h"

class Frustum {
private:
    glm::vec4 planes[6];

public:
    Frustum(const glm::mat4& viewProjMatrix);

    bool isBoxInside(const glm::vec3& min, const glm::vec3& max) const;

    bool isSphereInside(const glm::vec3& center, float radius) const;
};

void cullObjectsOutOfView(
    const std::vector<Renderable*>& meshes,
    std::vector<Renderable*>& outputBuffer,
    const glm::mat4& viewProj
);

#endif