#include "MovementComponent.h"

#include "Application.h"
#include "Logger.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/physics/collision/Collision.h"
#include "engine/scene/entities/Entity.h"
#include "game/GameInstance.h"
#include "game/world/World.h"

static constexpr float GRAVITY = 9.81f;
static constexpr float GROUND_CHECK_EPSILON = 0.01f;

MovementComponent::MovementComponent(Entity* owner)
    : owner(owner),
      m_mmode(MovementMode::Walk),
      m_velocity(0.0f),
      m_inputDirection(0.0f, 0.0f, -1.0f),
      m_responsiveness(10.0f),
      m_externalForces(0.0f),
      m_movementSpeed(0.0f),
      m_isGrounded(false),
      m_gravityEnabled(false) {}

void MovementComponent::update(float deltaTime) {
    if (owner) {
        if (m_mmode == MovementMode::Walk) {
            // Ignore y direction
            glm::vec3 desiredVelocity = glm::normalize(glm::vec3(m_inputDirection.x, 0.0f, m_inputDirection.z)) *
                                        m_movementSpeed;
            glm::vec3 velocityDelta = desiredVelocity - glm::vec3(m_velocity.x, 0.0f, m_velocity.z);

            glm::vec3 totalExternalForces = m_externalForces;
            if (m_gravityEnabled && !m_isGrounded) {
                totalExternalForces.y -= GRAVITY;
            }

            glm::vec3 inputAcceleration = velocityDelta * m_responsiveness;
            glm::vec3 totalAcceleration = inputAcceleration + totalExternalForces;

            m_velocity += totalAcceleration * deltaTime;
        } else if (m_mmode == MovementMode::Swim) {
            throw std::runtime_error("Swim movement mode not yet supported");
        } else if (m_mmode == MovementMode::Fly) {
            glm::vec3 desiredVelocity = m_inputDirection * m_movementSpeed;
            glm::vec3 velocityDelta = desiredVelocity - m_velocity;

            glm::vec3 totalExternalForces = m_externalForces;
            if (m_gravityEnabled && !m_isGrounded) {
                totalExternalForces.y -= GRAVITY;
            }

            glm::vec3 inputAcceleration = velocityDelta * m_responsiveness;
            glm::vec3 totalAcceleration = inputAcceleration + totalExternalForces;

            m_velocity += totalAcceleration * deltaTime;
        } else if (m_mmode == MovementMode::Spectator) {
            glm::vec3 desiredVelocity = m_inputDirection * m_movementSpeed;
            glm::vec3 velocityDelta = desiredVelocity - m_velocity;

            glm::vec3 inputAcceleration = velocityDelta * m_responsiveness;
            glm::vec3 totalAcceleration = inputAcceleration + m_externalForces;

            m_velocity += totalAcceleration * deltaTime;
        }

        if (m_mmode != MovementMode::Spectator) {
            glm::vec3 finalDelta = m_velocity * deltaTime;
            glm::vec3 pos = owner->getTransform().getPosition();

            // TODO Declare playerbox somewhere more fitting
            BoundingBox playerBox = {pos + glm::vec3(-0.2f, 0.0f, -0.2f), pos + glm::vec3(0.2f, 1.8f, 0.2f)};

            World* world = Application::getContext()->instance->m_world;

            // Correcting delta movement according to collisions with terrain
            finalDelta = sweepAndResolve(playerBox, finalDelta, world);

            // Is on ground when no y movement is happening and a small sweep in negative y gets diminished
            m_isGrounded = false;
            if (finalDelta.y == 0.0f) {
                m_isGrounded = sweepAndResolveAxis(
                                   playerBox.movedBy(finalDelta),
                                   glm::vec3(0.0f, -GROUND_CHECK_EPSILON, 0.0f),
                                   Axis::Y,
                                   world
                               ) > -GROUND_CHECK_EPSILON;
            }

            m_velocity = finalDelta / deltaTime;
        }
    }
}