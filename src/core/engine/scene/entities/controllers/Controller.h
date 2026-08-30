#ifndef TOOMANYBLOCKS_CONTROLLER_H
#define TOOMANYBLOCKS_CONTROLLER_H

#include "engine/Updatable.h"

class Entity;

class Controller : public Updatable {
protected:
    Entity* m_possessedEntity;

public:
    Controller() : m_possessedEntity(nullptr) {};
    virtual ~Controller();

    void possess(Entity* entity);
    void unpossess();
};

#endif