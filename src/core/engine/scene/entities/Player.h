#ifndef TOOMANYBLOCKS_PLAYER_H
#define TOOMANYBLOCKS_PLAYER_H

#include <glm/vec3.hpp>
#include <memory>

#include "engine/rendering/camera/Camera.h"
#include "engine/scene/SceneComponent.h"
#include "engine/scene/entities/Entity.h"

class Player : public Entity {
private:
    std::shared_ptr<Camera> m_camera;
    glm::ivec3 m_focusedBlock;
    bool m_isFocusingBlock;
    float m_reach;

public:
    Player();
    virtual ~Player() = default;

    inline std::shared_ptr<Camera> getCamera() const { return m_camera; }

    inline void setReachDistance(float reach) { m_reach = reach; }

    inline float getReachDistance() const { return m_reach; }

    inline void setFocusedBlock(const glm::ivec3& focusedBlock) { m_focusedBlock = focusedBlock; }

    inline glm::ivec3 getFocusedBlock() const { return m_focusedBlock; }

    inline void setIsFocusingBlock(bool isBlockFocused) { m_isFocusingBlock = isBlockFocused; }

    inline bool isFocusingBlock() const { return m_isFocusingBlock; }

    void update(float deltaTime) override;
};

#endif