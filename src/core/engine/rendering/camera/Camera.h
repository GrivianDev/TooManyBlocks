#ifndef TOOMANYBLOCKS_CAMERA_H
#define TOOMANYBLOCKS_CAMERA_H

#include <glm/glm.hpp>
#include "engine/scene/SceneComponent.h"

class Camera : public SceneComponent {
private:
    float m_fovy;
    float m_aspectRatio;
    float m_viewDistance;

public:
    Camera(float fovy, float aspectRatio);
    virtual ~Camera() = default;

    glm::mat4 getProjectionMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getViewProjMatrix();

    inline float getFovy() const { return m_fovy; }
    inline float getAspectRatio() const { return m_aspectRatio; }
    inline float getViewDistance() const { return m_viewDistance; }

    inline void setFovyRatio(float fovy) { m_fovy = fovy; }
    inline void setAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    inline void setViewDistance(float viewDistance) { m_viewDistance = viewDistance; }
};

#endif