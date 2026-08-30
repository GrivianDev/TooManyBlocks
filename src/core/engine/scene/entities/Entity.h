#ifndef TOOMANYBLOCKS_ENTITY_H
#define TOOMANYBLOCKS_ENTITY_H

#include "engine/Updatable.h"
#include "engine/scene/SceneComponent.h"
#include "engine/scene/Transform.h"
#include "engine/scene/entities/controllers/Controller.h"
#include "engine/scene/entities/movement/MovementComponent.h"

class MovementComponent;

class Entity : public Updatable {
    friend class Controller;

protected:
    SceneComponent m_sceneRoot;
    MovementComponent* m_movement;
    Controller* m_controller;

public:
    Entity();
    virtual ~Entity();

    void update(float deltaTime) override;

    inline Transform& getTransform() { return m_sceneRoot.getLocalTransform(); }

    glm::vec3 getVelocity() const;

    inline MovementComponent* getMovementComponent() const { return m_movement; }

    inline Controller* getController() const { return m_controller; }

    inline bool isPossessed() const { return m_controller != nullptr; }
};

#endif
