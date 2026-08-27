#ifndef TOOMANYBLOCKS_ANIMATIONCONTROLLER_H
#define TOOMANYBLOCKS_ANIMATIONCONTROLLER_H

#include <memory>
#include <vector>

#include "engine/Updatable.h"
#include "engine/animation/AnimationController.h"
#include "engine/animation/AnimationLayer.h"

class AnimationController : public Updatable {
private:
    class SkeletalMesh* m_owner;

    std::vector<AnimationLayer> m_layers;

    std::vector<Transform> m_evaluationTransforms;

    void evaluate();

public:
    AnimationController(SkeletalMesh* owner);

    AnimationLayer& addLayer(std::unique_ptr<AnimationNode> node);
    AnimationLayer& getLayer(size_t index);

    const std::vector<Transform>& getEvaluationTransforms() const { return m_evaluationTransforms; }

    void update(float deltaTime) override;
};

#endif
