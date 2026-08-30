#ifndef TOOMANYBLOCKS_BLENDSPACEPLAYER1D_H
#define TOOMANYBLOCKS_BLENDSPACEPLAYER1D_H

#include <functional>
#include <memory>
#include <vector>
#include "engine/scene/Transform.h"
#include "engine/animation/AnimationNode.h"
#include "engine/scene/renderables/SkeletalMesh.h"

class BlendSpacePlayer1D : public AnimationNode {
public:
    struct Sample {
        float position;
        std::unique_ptr<AnimationNode> node;
    };

private:
    SkeletalMesh* m_owner;

    std::vector<Sample> m_samples;

    std::function<float(const SkeletalMesh*)> m_positionFunction;

    std::vector<Transform> m_poseA;
    std::vector<Transform> m_poseB;

    float m_position;
    float m_speed;

public:
    BlendSpacePlayer1D(SkeletalMesh* owner, std::function<float(const SkeletalMesh*)> positionFunction = nullptr);

    inline void setPositionFunction(std::function<float(const SkeletalMesh*)> positionFunction) {
        m_positionFunction = std::move(positionFunction);
    }
    void addSample(float position, std::unique_ptr<AnimationNode> node);
    inline void setPosition(float position) { m_position = position; }
    inline void setSpeed(float speed) { m_speed = speed; }

    inline const std::vector<Sample>& getSamples() const { return m_samples; }
    inline float getPosition() const { return m_position; }
    inline float getSpeed() const { return m_speed; }

    void update(float deltaTime) override;

    void evaluate(std::vector<Transform>& outPose) override;
};

#endif
