#include "AnimationController.h"

#include "engine/scene/renderables/SkeletalMesh.h"

void AnimationController::evaluate() {
    m_evaluationTransforms.resize(m_owner->getNodeCount());

    // Start with the skeletons resting pose
    const std::vector<Node>& nodeArray = m_owner->getAssetHandle().value().nodeArray;
    for (size_t i = 0; i < nodeArray.size(); i++) {
        m_evaluationTransforms[i] = nodeArray[i].localTransform;
    }

    // Apply layers in order
    for (AnimationLayer& layer : m_layers) {
        layer.evaluate(m_evaluationTransforms);
    }
}

AnimationController::AnimationController(SkeletalMesh* owner) : m_owner(owner) {}

AnimationLayer& AnimationController::addLayer(std::unique_ptr<AnimationNode> node) {
    return m_layers.emplace_back(m_owner->getNodeCount(), std::move(node));
}

AnimationLayer& AnimationController::getLayer(size_t index) { return m_layers[index]; }

void AnimationController::update(float deltaTime) {
    if (!m_owner || !m_owner->isReady()) return;

    for (AnimationLayer& layer : m_layers) {
        layer.update(deltaTime);
    }

    evaluate();
}
