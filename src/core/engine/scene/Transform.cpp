#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/scene/Axis.h"

void Transform::recalculateModelMatrix() const {
    glm::mat3 rot = glm::mat3_cast(m_rotation);
    rot *= m_scale;  // Bake in scale

    // Directly reassign values in place
    m_modelMatrix[0] = glm::vec4(rot[0], 0.0f);
    m_modelMatrix[1] = glm::vec4(rot[1], 0.0f);
    m_modelMatrix[2] = glm::vec4(rot[2], 0.0f);
    m_modelMatrix[3] = glm::vec4(m_position, 1.0f);

    // Commented out original equivalent but slower version cause of matrix multiplications overhead
    // m_modelMatrix = glm::mat4(1.0f);
    // m_modelMatrix = glm::translate(m_modelMatrix, m_position) * glm::mat4_cast(m_rotation) *
    //                 glm::scale(m_modelMatrix, glm::vec3(m_scale));

    m_dirty = false;
}

Transform Transform::fromMatrix(const glm::mat4& matrix) {
    glm::vec3 scale(glm::length(matrix[0]), glm::length(matrix[1]), glm::length(matrix[2]));
    return Transform(glm::vec3(matrix[3]), glm::normalize(glm::quat_cast(glm::mat3(matrix))), scale.x);
}

Transform::Transform()
    : m_position(0.0f),
      m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      m_scale(1.0f),
      m_modelMatrix(1.0f),
      m_dirty(true) {}

Transform::Transform(const glm::vec3& position, const glm::vec3& eulerAngles, float scale)
    : m_position(position), m_rotation(glm::radians(eulerAngles)), m_scale(scale), m_modelMatrix(1.0f), m_dirty(true) {}

Transform::Transform(const glm::vec3& position, const glm::quat& rotation, float scale)
    : m_position(position), m_rotation(rotation), m_scale(scale), m_modelMatrix(1.0f), m_dirty(true) {}

void Transform::rotate(float angle, const glm::vec3& axis) {
    glm::quat rotationQuat = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
    m_rotation = glm::normalize(rotationQuat * m_rotation);
    m_dirty = true;
}

void Transform::rotate(const glm::vec3& eulerAngles) {
    glm::quat rotationQuat = glm::quat(glm::radians(eulerAngles));
    m_rotation = glm::normalize(rotationQuat * m_rotation);
    m_dirty = true;
}

void Transform::rotate(const glm::quat& quaternion) {
    m_rotation = glm::normalize(quaternion * m_rotation);
    m_dirty = true;
}

void Transform::translate(const glm::vec3& offset) {
    m_position += offset;
    m_dirty = true;
}

void Transform::scale(float factor) {
    m_scale *= factor;
    m_dirty = true;
}

void Transform::lookAt(const glm::vec3& target, const glm::vec3& up) {
    // Alternative
    // glm::mat4 lookAtMatrix = glm::lookAt(m_position, target, up);
    // m_rotation = glm::quat_cast(lookAtMatrix);
    
    glm::vec3 forward = glm::normalize(target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 newUp = glm::cross(right, forward);

    glm::mat3 rotationMatrix;
    rotationMatrix[0] = right;
    rotationMatrix[1] = newUp;
    rotationMatrix[2] = -forward;

    m_rotation = glm::quat_cast(rotationMatrix);
    m_dirty = true;
}

void Transform::reset() {
    m_position = glm::vec3(0.0f);
    m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_scale = 1.0f;
    m_dirty = true;
}

void Transform::setPosition(const glm::vec3& position) {
    m_position = position;
    m_dirty = true;
}

void Transform::setRotation(const glm::quat& rotation) {
    m_rotation = rotation;
    m_dirty = true;
}

void Transform::setRotation(const glm::vec3& eulerAngles) {
    m_rotation = glm::quat(glm::radians(eulerAngles));
    m_dirty = true;
}

void Transform::setScale(float scale) {
    m_scale = scale;
    m_dirty = true;
}

glm::vec3 Transform::getForward() const {
    return glm::normalize(m_rotation * WorldForward);  // Forward is -Z in local space
}

glm::vec3 Transform::getRight() const {
    return glm::normalize(m_rotation * WorldRight);  // Right is +X in local space
}

glm::vec3 Transform::getUp() const {
    return glm::normalize(m_rotation * WorldUp);  // Up is +Y in local space
}

Transform Transform::getInverse() const {
    Transform result;
    result.setPosition(-m_position);
    result.setRotation(glm::conjugate(m_rotation));
    result.setScale(1.0f / m_scale);
    return result;
}

bool Transform::isEqual(const Transform& other, float epsilon) const {
    return glm::all(glm::lessThan(glm::abs(m_position - other.m_position), glm::vec3(epsilon))) &&
           glm::abs(glm::dot(m_rotation, other.m_rotation) - 1.0f) < epsilon &&
           glm::abs(m_scale - other.m_scale) < epsilon;
}

glm::mat4 Transform::getModelMatrix() const {
    if (m_dirty) {
        recalculateModelMatrix();
    }
    return m_modelMatrix;
}

Transform Transform::interpolate(const Transform& other, float a) const {
    Transform result;
    result.setPosition(glm::mix(getPosition(), other.getPosition(), a));
    result.setRotation(
        glm::slerp(getRotationQuat(), other.getRotationQuat(), a)
    );  // Spherical linear interpolation for quaternions
    result.setScale(glm::mix(getScale(), other.getScale(), a));
    return result;
}

Transform Transform::operator+(const Transform& other) const {
    Transform result;
    result.setPosition(getPosition() + other.getPosition());
    result.setRotation(getRotationQuat() * other.getRotationQuat());
    result.setScale(getScale() + other.getScale());
    return result;
}

Transform& Transform::operator+=(const Transform& other) {
    setPosition(getPosition() + other.getPosition());
    setRotation(getRotationQuat() * other.getRotationQuat());
    setScale(getScale() + other.getScale());
    return *this;
}

Transform Transform::operator*(const Transform& other) const {
    Transform result;
    result.setPosition(getPosition() + (getRotationQuat() * (other.getPosition() * getScale())));
    result.setRotation(getRotationQuat() * other.getRotationQuat());
    result.setScale(getScale() * other.getScale());
    return result;
}

Transform& Transform::operator*=(const Transform& other) {
    setPosition(getPosition() + (getRotationQuat() * (other.getPosition() * getScale())));
    setRotation(getRotationQuat() * other.getRotationQuat());
    setScale(getScale() * other.getScale());
    return *this;
}

bool Transform::operator==(const Transform& other) const {
    return m_position == other.m_position && m_rotation == other.m_rotation && m_scale == other.m_scale;
}

bool Transform::operator!=(const Transform& other) const { return !(*this == other); }
