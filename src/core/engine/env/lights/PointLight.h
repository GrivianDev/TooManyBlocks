#ifndef TOOMANYBLOCKS_POINTLIGHT_H
#define TOOMANYBLOCKS_POINTLIGHT_H

#include "engine/env/lights/Light.h"

class PointLight : public Light {
public:
    PointLight(const glm::vec3& color, float intensity, float range)
        : Light(LightType::Point, color, intensity, range) {};
    virtual ~PointLight() = default;

    virtual inline unsigned int faceCount() const override { return 6; }
};

#endif
