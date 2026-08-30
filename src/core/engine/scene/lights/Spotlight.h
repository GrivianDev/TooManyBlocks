#ifndef TOOMANYBLOCKS_SPOTLIGHT_H
#define TOOMANYBLOCKS_SPOTLIGHT_H

#include <algorithm>

#include "engine/scene/lights/Light.h"

class Spotlight : public Light {
public:
    Spotlight(const glm::vec3& color, float intensity, float fovy, float range);
    virtual ~Spotlight() = default;

    inline float getFovy() const { return m_internal.fovy; }
    inline float getInnerCutoffAngle() const { return m_internal.innerCutoffAngle; }

    inline void setFovy(float fovy) {
        m_internal.fovy = fovy;
        m_internal.innerCutoffAngle = std::min<float>(m_internal.innerCutoffAngle, fovy);
    }
    inline void setInnerCutoffAngle(float angle) {
        m_internal.innerCutoffAngle = std::min<float>(angle, m_internal.fovy);
    }

    virtual inline unsigned int faceCount() const override { return 1; }
};

#endif