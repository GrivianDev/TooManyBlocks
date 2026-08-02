#ifndef TOOMANYBLOCKS_DIRECTIONALLIGHT_H
#define TOOMANYBLOCKS_DIRECTIONALLIGHT_H

#include "engine/env/lights/Light.h"

constexpr unsigned int MAX_SHADOW_CASCADES = 4;

class DirectionalLight : public Light {
private:
    unsigned int m_cascadeCount;
    float m_cascadeLambda;
    float m_shadowDistance;

public:
    DirectionalLight(const glm::vec3& color, float intensity)
        : Light(LightType::Directional, color, intensity, 0.0f),
          m_cascadeCount(1),
          m_cascadeLambda(0.0f),
          m_shadowDistance(200.0f) {}
    virtual ~DirectionalLight() = default;

    inline void setCascadeCount(unsigned int cascadeCount) { m_cascadeCount = cascadeCount; }
    inline void setCascadeLambda(float lambda) { m_cascadeLambda = lambda; }
    inline void setShadowDistance(float distance) { m_shadowDistance = distance; }

    inline unsigned int getCascadeCount() const { return m_cascadeCount; }
    // lambda=0 -> linear cascade split / lambda=1 -> logarithmic cascade split
    inline float getCascadeLambda() const { return m_cascadeLambda; }
    // Range that shadows are casted (far plane of last shadow cascade)
    inline float getShadowDistance() const { return m_shadowDistance; }

    virtual inline unsigned int faceCount() const override { return m_cascadeCount; }
};

#endif