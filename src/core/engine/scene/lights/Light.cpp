#include "Light.h"

Light::Light(LightType type, const glm::vec3& color, float intensity, float range)
    : m_internal({}), m_shadowPriority(1.0f), m_castsShadows(false) {
    m_internal.lightType = static_cast<unsigned int>(type);
    m_internal.color = color;
    m_internal.intensity = intensity;
    m_internal.range = range;
}

GPULight Light::toGPULight() {
    Transform tr = getGlobalTransform();
    m_internal.lightPosition = tr.getPosition();
    m_internal.direction = tr.getForward();
    return m_internal;
}
