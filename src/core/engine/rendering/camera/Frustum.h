#ifndef TOOMANYBLOCKS_FRUSTUM_H
#define TOOMANYBLOCKS_FRUSTUM_H

#include <glm/glm.hpp>
#include <vector>

#include "engine/geometry/BoundingVolume.h"
#include "engine/scene/renderables/Renderable.h"

class Frustum {
private:
    glm::vec4 planes[6];
    BoundingBox m_bounds;

public:
    Frustum() = default;
    explicit Frustum(const glm::mat4& viewProjMatrix);

    // May produce false positives rarely
    bool isBoxPotentiallyInside(const glm::vec3& min, const glm::vec3& max) const;

    // May produce false positives rarely
    bool isSpherePotentiallyInside(const glm::vec3& center, float radius) const;

    inline const BoundingBox& getBounds() const { return m_bounds; }
};

bool isObjectInView(const Renderable* renderable, const Frustum& frustum);

#endif