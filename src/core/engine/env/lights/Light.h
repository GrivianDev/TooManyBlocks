#ifndef TOOMANYBLOCKS_LIGHT_H
#define TOOMANYBLOCKS_LIGHT_H

#include <engine/comp/SceneComponent.h>

#include <glm/glm.hpp>

constexpr unsigned int MAX_LIGHTS = 1024;
constexpr unsigned int SHADOW_ATLAS_COUNT = 4;

enum class LightType {
    Directional,
    Spot,
    Point
};

struct alignas(16) GPULight {
    unsigned int lightType;
    unsigned int shadowMapsOffset;
    unsigned int shadowMapCount;
    float intensity;
    glm::vec3 lightPosition;
    float range;  // Used by point- / spotlight
    glm::vec3 direction;
    float fovy;  // Used by spotlicht
    glm::vec3 color;
    float innerCutoffAngle;  // Used by spotlicht
};

struct alignas(16) GPUShadowMap {
    glm::mat4 viewProjection;

    unsigned int lightIndex;
    unsigned int atlasIndex;
    unsigned int resolution;
    float cascadeSplit;

    glm::vec2 atlasOffset;  // [0, 1] Normalized to atlas size
    glm::vec2 atlasScale;   // [0, 1] Normalized to atlas size
};

class Light : public SceneComponent {
    friend class LightProcessor;

protected:
    GPULight m_internal;

    float m_shadowPriority;
    bool m_castsShadows;

    inline void setShadowMapsOffset(unsigned int offset) { m_internal.shadowMapsOffset = offset; }
    inline void setShadowMapCount(unsigned int count) { m_internal.shadowMapCount = count; }

public:
    Light(LightType type, const glm::vec3& color, float intensity, float range);
    virtual ~Light() = default;

    inline LightType getType() const { return static_cast<LightType>(m_internal.lightType); }
    inline glm::vec3 getColor() const { return m_internal.color; }
    inline float getIntensity() const { return m_internal.intensity; }
    inline float getRange() const { return m_internal.range; }
    inline float getShadowPriority() const { return m_shadowPriority; }
    inline int getShadowMapsOffset() const { return m_internal.shadowMapsOffset; }
    inline int getShadowMapCount() const { return m_internal.shadowMapCount; }
    inline bool castsShadows() const { return m_castsShadows; }
    inline bool hasShadowAllocation() const { return m_internal.shadowMapCount > 0; }

    inline void setColor(const glm::vec3& color) { m_internal.color = color; }
    inline void setIntensity(float intensity) { m_internal.intensity = intensity; }
    inline void setRange(float range) { m_internal.range = range; }
    inline void setShadowPriority(float priority) { m_shadowPriority = priority; }
    inline void setCastsShadows(bool enabled) { m_castsShadows = enabled; };

    virtual GPULight toGPULight();
    virtual unsigned int faceCount() const = 0;
};

#endif
