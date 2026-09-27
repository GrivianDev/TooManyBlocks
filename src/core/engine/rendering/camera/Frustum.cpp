#include "Frustum.h"

#include <algorithm>

#include "engine/geometry/BoundingVolume.h"

// OpenGL NDC
static constexpr glm::vec3 corners[8] = {
    {-1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {-1.0f, 1.0f, -1.0f},
    {1.0f, 1.0f, -1.0f},

    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f}
};

enum Planes {
    Near,
    Far,
    Left,
    Right,
    Top,
    Bottom
};

Frustum::Frustum(const glm::mat4& viewProjMatrix) : m_bounds(BoundingBox::invalid()) {
    // Extract planes from the combined projection-view viewProjMatrix
    planes[Left] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][0],
        viewProjMatrix[1][3] + viewProjMatrix[1][0],
        viewProjMatrix[2][3] + viewProjMatrix[2][0],
        viewProjMatrix[3][3] + viewProjMatrix[3][0]
    );
    planes[Right] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][0],
        viewProjMatrix[1][3] - viewProjMatrix[1][0],
        viewProjMatrix[2][3] - viewProjMatrix[2][0],
        viewProjMatrix[3][3] - viewProjMatrix[3][0]
    );
    planes[Bottom] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][1],
        viewProjMatrix[1][3] + viewProjMatrix[1][1],
        viewProjMatrix[2][3] + viewProjMatrix[2][1],
        viewProjMatrix[3][3] + viewProjMatrix[3][1]
    );
    planes[Top] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][1],
        viewProjMatrix[1][3] - viewProjMatrix[1][1],
        viewProjMatrix[2][3] - viewProjMatrix[2][1],
        viewProjMatrix[3][3] - viewProjMatrix[3][1]
    );
    planes[Near] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][2],
        viewProjMatrix[1][3] + viewProjMatrix[1][2],
        viewProjMatrix[2][3] + viewProjMatrix[2][2],
        viewProjMatrix[3][3] + viewProjMatrix[3][2]
    );
    planes[Far] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][2],
        viewProjMatrix[1][3] - viewProjMatrix[1][2],
        viewProjMatrix[2][3] - viewProjMatrix[2][2],
        viewProjMatrix[3][3] - viewProjMatrix[3][2]
    );

    // Normalize the planes
    for (int i = 0; i < 6; i++) {
        planes[i] /= glm::length(glm::vec3(planes[i]));
    }

    const glm::mat4 invVP = glm::inverse(viewProjMatrix);
    for (const glm::vec3& corner : corners) {
        glm::vec4 world = invVP * glm::vec4(corner, 1.0f);

        // Perspective divide
        world /= world.w;

        m_bounds.min = glm::min(m_bounds.min, glm::vec3(world));
        m_bounds.max = glm::max(m_bounds.max, glm::vec3(world));
    }
}

bool Frustum::isBoxPotentiallyInside(const glm::vec3& min, const glm::vec3& max) const {
    for (int i = 0; i < 6; i++) {
        const glm::vec4& plane = planes[i];

        // Find the corner of the AABB that is furthest in the
        // direction of the plane normal. If even this corner is
        // outside the plane, the entire AABB is outside the frustum.
        // Otherwise, the AABB is only known to be a potential intersection.
        glm::vec3 positiveCorner = glm::vec3(
            (plane.x > 0) ? max.x : min.x, (plane.y > 0) ? max.y : min.y, (plane.z > 0) ? max.z : min.z
        );

        if (glm::dot(glm::vec3(plane), positiveCorner) + plane.w < 0) {
            // Entire box is outside the frustum (Guranteed)
            return false;
        }
    }
    return true;
}

bool Frustum::isSpherePotentiallyInside(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius) {
            // The entire sphere is outside this frustum plane (Guranteed)
            return false;
        }
    }
    return true;
}

bool isObjectInView(const Renderable* renderable, const Frustum& frustum) {
    const BoundingBox& bounds = renderable->getGlobalBounds();
    if (bounds.isInvalid()) {
        return false;
    }
     return !renderable->isCullable() || frustum.isBoxPotentiallyInside(bounds.min, bounds.max);
}
