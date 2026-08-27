#include "AnimationLayer.h"

#include <algorithm>

#include "Logger.h"

AnimationLayer::AnimationLayer(size_t nodeCount, std::unique_ptr<AnimationNode> node)
    : m_node(std::move(node)), m_layerPose(nodeCount), m_mask(nodeCount), m_weight(1.0f) {}

void AnimationLayer::update(float deltaTime) {
    if (!m_node) return;
    m_node->update(deltaTime);
}

void AnimationLayer::evaluate(std::vector<Transform>& outPose) {
    if (!m_node || m_weight <= 0.0f) return;

    m_layerPose.resize(outPose.size());
    m_node->evaluate(m_layerPose);

    for (size_t i = 0; i < outPose.size(); i++) {
        if (m_mask.getWeights()[i] <= 0.0f) {
            continue;
        }
        outPose[i] = outPose[i].interpolate(m_layerPose[i], m_mask.getWeights()[i] * m_weight);
    }
}
