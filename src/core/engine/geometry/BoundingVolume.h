#ifndef TOOMANYBLOCKS_BOUNDINGVOLUME_H
#define TOOMANYBLOCKS_BOUNDINGVOLUME_H

#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct BoundingSphere {
    glm::vec3 center;
    float radius;

    static constexpr BoundingSphere invalid() { return {glm::vec3(0), -FLT_MAX}; }

    inline BoundingSphere movedBy(const glm::vec3& delta) const { return {center + delta, radius}; }

    inline BoundingSphere scaledBy(float delta) const { return {center, radius + delta}; }

    inline bool isInvalid() const { return *this == invalid(); }

    inline float surfaceArea() const { return 4.0f * glm::pi<float>() * radius * radius; }

    inline bool operator==(const BoundingSphere& other) const {
        return center == other.center && radius == other.radius;
    }

    inline bool operator!=(const BoundingSphere& other) const { return !(*this == other); }
};

// Axis Aligned Bounding Box
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    static constexpr BoundingBox invalid() { return {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)}; }

    static constexpr BoundingBox combined(const BoundingBox& a, const BoundingBox& b) {
        return {glm::min(a.min, b.min), glm::max(a.max, b.max)};
    }

    inline glm::vec3 center() const { return (min + max) * 0.5f; }

    inline BoundingBox movedBy(const glm::vec3& delta) const { return {min + delta, max + delta}; }

    inline BoundingBox extendedBy(float margin) const { return {min - margin, max + margin}; }

    inline bool contains(const BoundingBox& other) const {
        return min.x <= other.min.x && min.y <= other.min.y && min.z <= other.min.z && max.x >= other.max.x &&
               max.y >= other.max.y && max.z >= other.max.z;
    }

    inline bool overlaps(const BoundingBox other) const {
        return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    inline bool isInvalid() const { return *this == invalid(); }

    inline float surfaceArea() const {
        glm::vec3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    inline bool operator==(const BoundingBox& other) const { return min == other.min && max == other.max; }

    inline bool operator!=(const BoundingBox& other) const { return !(*this == other); }
};

#endif
