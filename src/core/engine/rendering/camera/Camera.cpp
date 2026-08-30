#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include "engine/rendering/camera/Camera.h"

Camera::Camera(float fovy, float aspectRatio) : m_fovy(fovy), m_aspectRatio(aspectRatio), m_viewDistance(500.0f) {}

glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspective(glm::radians(m_fovy), m_aspectRatio, 0.1f, m_viewDistance);
}

glm::mat4 Camera::getViewMatrix() {
    Transform tr = getGlobalTransform();
    glm::vec3 position = tr.getPosition();

    return glm::lookAt(position, position + tr.getForward(), tr.getUp());
}

glm::mat4 Camera::getViewProjMatrix() { return getProjectionMatrix() * getViewMatrix(); }