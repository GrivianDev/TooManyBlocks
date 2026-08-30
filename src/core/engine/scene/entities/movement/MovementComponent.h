#ifndef TOOMANYBLOCKS_MOVEMENTCOMPONENT_H
#define TOOMANYBLOCKS_MOVEMENTCOMPONENT_H

#include <glm/glm.hpp>

#include "engine/Updatable.h"

class Entity;

enum class MovementMode {
    Walk,
    Fly,
    Swim,
    Spectator
};

class MovementComponent : public Updatable {
private:
    Entity* owner;
    MovementMode m_mmode;
    glm::vec3 m_velocity;
    glm::vec3 m_inputDirection;
    float m_responsiveness;
    glm::vec3 m_externalForces;
    float m_movementSpeed;
    bool m_isGrounded;
    bool m_gravityEnabled;

public:
    MovementComponent(Entity* owner = nullptr);
    virtual ~MovementComponent() = default;

    inline MovementMode getMovementMode() const { return m_mmode; }
    inline glm::vec3 getVelocity() const { return m_velocity; }
    inline glm::vec3 getInputDirection() const { return m_inputDirection; }
    inline float getResponsiveness() const { return m_responsiveness; }
    inline glm::vec3 getExternalForces() const { return m_externalForces; }
    inline float getMovementSpeed() const { return m_movementSpeed; }
    inline bool isGrounded() const { return m_isGrounded; }
    inline bool isGravityEnabled() const { return m_gravityEnabled; }

    inline void setMovementMode(MovementMode mode) { m_mmode = mode; }
    inline void setVelocity(const glm::vec3& velocity) { m_velocity = velocity; }
    inline void setInputDirection(const glm::vec3& direction) {
        if (glm::length(direction) > 0.0f) m_inputDirection = glm::normalize(direction);
    }
    inline void setResponsiveness(float responsiveness) { m_responsiveness = responsiveness; }
    inline void setExternalForces(const glm::vec3& forces) { m_externalForces = forces; }
    inline void setMovementSpeed(float speed) { m_movementSpeed = speed; }
    inline void setGravityEnabled(bool enabled) { m_gravityEnabled = enabled; }

    inline void addImpulse(const glm::vec3& impulse) { m_velocity += impulse; }
    inline void addForce(const glm::vec3& force) { m_externalForces += force; }

    void update(float deltaTime) override;
};

#endif