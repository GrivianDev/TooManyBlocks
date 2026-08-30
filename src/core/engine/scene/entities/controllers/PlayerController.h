#ifndef TOOMANYBLOCKS_PLAYERCONTROLLER_H
#define TOOMANYBLOCKS_PLAYERCONTROLLER_H

#include <unordered_map>

#include "platform/input/KeyMouseIO.h"
#include "engine/scene/entities/controllers/Controller.h"

class PlayerController : public Controller, public KeyObserver, public MouseObserver {
private:
    std::unordered_map<int, bool> keyStates;
    float m_cameraPitch;
    bool m_playerNeedsReadjustment;

public:
    PlayerController();
    virtual ~PlayerController();

    void notify(KeyEvent event, KeyEventData data) override;

    void notify(MousEvent event, MouseEventData data) override;

    void update(float deltaTime) override;
};

#endif