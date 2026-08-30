#ifndef TOOMANYBLOCKS_TRANSFORM_H
#define TOOMANYBLOCKS_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    float m_scale;  // Uniform scaling to avoid skew

    mutable glm::mat4 m_modelMatrix;
    mutable bool m_dirty;  // Tracks whether the model matrix needs recalculation

    void recalculateModelMatrix() const;

public:
    static Transform fromMatrix(const glm::mat4& matrix);

    Transform();
    Transform(const glm::vec3& position, const glm::vec3& eulerAngles = glm::vec3(0.0f), float scale = 1.0f);
    Transform(
        const glm::vec3& position, const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), float scale = 1.0f
    );

    void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::vec3& eulerAngles);
    void rotate(const glm::quat& quaternion);
    void translate(const glm::vec3& offset);
    void scale(float factor);
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
    void reset();

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setRotation(const glm::vec3& eulerAngles);
    void setScale(float scale);

    inline glm::vec3 getPosition() const { return m_position; }
    inline glm::quat getRotationQuat() const { return m_rotation; }
    inline glm::vec3 getRotationEuler() const { return glm::degrees(glm::eulerAngles(m_rotation)); };
    inline float getScale() const { return m_scale; }
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    Transform getInverse() const;
    bool isEqual(const Transform& other, float epsilon = 1e-5f) const;

    glm::mat4 getModelMatrix() const;

    Transform interpolate(const Transform& other, float a) const;

    Transform operator+(const Transform& other) const;
    Transform& operator+=(const Transform& other);
    Transform operator*(const Transform& other) const;
    Transform& operator*=(const Transform& other);
    bool operator==(const Transform& other) const;
    bool operator!=(const Transform& other) const;
};

#endif