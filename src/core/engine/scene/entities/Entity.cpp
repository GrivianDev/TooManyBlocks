#include "Entity.h"

#include "Application.h"
#include "game/GameInstance.h"

Entity::Entity() : m_movement(new MovementComponent(this)) {}

Entity::~Entity() {
    if (m_controller) {
        m_controller->unpossess();
    }
    delete m_movement;
}

void Entity::update(float deltaTime) {
    m_movement->update(deltaTime);
    glm::vec3 deltaDistance = m_movement->getVelocity() * deltaTime;
    m_sceneRoot.getLocalTransform().translate(deltaDistance);
}

glm::vec3 Entity::getVelocity() const { return m_movement->getVelocity(); }