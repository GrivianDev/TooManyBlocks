#include "Frustum.h"

#include <algorithm>

#include "engine/geometry/BoundingVolume.h"

enum Planes {
    Near,
    Far,
    Left,
    Right,
    Top,
    Bottom
};

Frustum::Frustum(const glm::mat4& viewProjMatrix) {
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
}

bool Frustum::isBoxInside(const glm::vec3& min, const glm::vec3& max) const {
    for (int i = 0; i < 6; i++) {
        const glm::vec4& plane = planes[i];

        // Check if all corners of the bounding box are outside the plane
        glm::vec3 positiveCorner = glm::vec3(
            (plane.x > 0) ? max.x : min.x, (plane.y > 0) ? max.y : min.y, (plane.z > 0) ? max.z : min.z
        );

        if (glm::dot(glm::vec3(plane), positiveCorner) + plane.w < 0) {
            return false;  // Box is outside the frustum
        }
    }
    return true;  // Box is inside or intersects the frustum
}

bool Frustum::isSphereInside(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius) {
            return false;
        }
    }
    return true;
}

void cullObjectsOutOfView(
    const std::vector<Renderable*>& meshes,
    std::vector<Renderable*>& outputBuffer,
    const glm::mat4& viewProj
) {
    const Frustum frustum(viewProj);

    outputBuffer.clear();
    for (Renderable* mesh : meshes) {
        const BoundingBox bounds = mesh->getBoundingBox();
        if (!bounds.isInvalid() || bounds.isNotCullable()) {
            // TODO This is a quickfix for correct culling, maybe make this nicer
            Transform transform = mesh->getGlobalTransform();

            const glm::vec3 localCenter = (bounds.min + bounds.max) * 0.5f;
            const glm::vec3 localExtents = (bounds.max - bounds.min) * 0.5f;

            const glm::vec3 worldCenter = transform.getPosition() +
                                          transform.getRotationQuat() * (localCenter * transform.getScale());

            const glm::mat3 rotation = glm::mat3_cast(transform.getRotationQuat());

            const glm::mat3 absRotation(glm::abs(rotation[0]), glm::abs(rotation[1]), glm::abs(rotation[2]));

            const glm::vec3 worldExtents = absRotation * (localExtents * transform.getScale());

            const glm::vec3 worldMin = worldCenter - worldExtents;
            const glm::vec3 worldMax = worldCenter + worldExtents;

            if (frustum.isBoxInside(worldMin, worldMax)) {
                outputBuffer.push_back(mesh);
            }
        }
    }
}