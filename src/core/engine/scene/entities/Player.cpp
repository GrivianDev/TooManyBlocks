#include "Player.h"

Player::Player() : m_camera(std::make_shared<Camera>(45.0f, 950.0f / 540.0f)), m_reach(6.0f) {
    m_sceneRoot.attachChild(m_camera.get());
    m_camera->getLocalTransform().translate(glm::vec3(0, 1.5, 0));
    m_movement->setMovementMode(MovementMode::Walk);
    m_movement->setGravityEnabled(true);
}

void Player::update(float deltaTime) {
    m_controller->update(deltaTime);
    m_movement->update(deltaTime);
    Entity::update(deltaTime);
}