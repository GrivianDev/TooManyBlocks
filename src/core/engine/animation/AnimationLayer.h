#ifndef TOOMANYBLOCKS_ANIMATIONLAYER_H
#define TOOMANYBLOCKS_ANIMATIONLAYER_H

#include <algorithm>
#include <memory>
#include <vector>

#include "datatypes/Transform.h"
#include "engine/animation/AnimationNode.h"
#include "engine/animation/BoneMask.h"

class AnimationLayer : public AnimationNode {
private:
    std::unique_ptr<AnimationNode> m_node;

    std::vector<Transform> m_layerPose;

    BoneMask m_mask;
    float m_weight;

public:
    AnimationLayer(size_t nodeCount, std::unique_ptr<AnimationNode> node = nullptr);
    AnimationLayer(AnimationLayer&&) = default;
    virtual ~AnimationLayer() = default;

    inline void setNode(std::unique_ptr<AnimationNode> node) { m_node = std::move(node); };
    inline void setMask(const BoneMask& mask) { m_mask = mask; };
    inline void setWeight(float weight) { m_weight = std::clamp<float>(weight, 0.0f, 1.0f); };

    inline const AnimationNode* getNode() const { return m_node.get(); }
    inline const BoneMask& getMask() const { return m_mask; }
    inline float getWeight() const { return m_weight; }

    void update(float deltaTime) override;

    void evaluate(std::vector<Transform>& outPose) override;

    AnimationLayer& operator=(AnimationLayer&&) = default;
};

#endif
