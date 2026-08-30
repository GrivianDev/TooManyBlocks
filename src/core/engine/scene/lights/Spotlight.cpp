#include "Spotlight.h"

Spotlight::Spotlight(const glm::vec3& color, float intensity, float fovy, float range)
    : Light(LightType::Spot, color, intensity, range) {
    m_internal.fovy = fovy;
    m_internal.innerCutoffAngle = fovy;
}
