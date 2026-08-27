#include "BlendSpacePlayer1D.h"

#include <algorithm>

BlendSpacePlayer1D::BlendSpacePlayer1D(SkeletalMesh* owner, std::function<float(const SkeletalMesh*)> positionFunction)
    : m_owner(owner), m_position(0.0f), m_speed(1.0f), m_positionFunction(std::move(positionFunction)) {}

void BlendSpacePlayer1D::addSample(float position, std::unique_ptr<AnimationNode> node) {
    auto it = std::lower_bound(m_samples.begin(), m_samples.end(), position, [](const Sample& sample, float position) {
        return sample.position < position;
    });

    m_samples.insert(it, Sample{position, std::move(node)});
}

void BlendSpacePlayer1D::update(float deltaTime) {
    if (m_owner && m_positionFunction) {
        m_position = m_positionFunction(m_owner);
    }

    for (Sample& sample : m_samples) {
        if (sample.node) sample.node->update(deltaTime * m_speed);
    }
}

void BlendSpacePlayer1D::evaluate(std::vector<Transform>& outPose) {
    if (m_samples.empty()) return;

    // Clamp to first or last
    if (m_position <= m_samples.front().position) {
        m_samples.front().node->evaluate(outPose);
        return;
    }
    if (m_position >= m_samples.back().position) {
        m_samples.back().node->evaluate(outPose);
        return;
    }

    // Find the two surrounding samples
    size_t right = 1;
    while (right < m_samples.size() && m_samples[right].position < m_position) {
        right++;
    }

    size_t left = right - 1;

    const Sample& a = m_samples[left];
    const Sample& b = m_samples[right];

    const float alpha = (m_position - a.position) / (b.position - a.position);

    m_poseA.resize(outPose.size());
    m_poseB.resize(outPose.size());

    a.node->evaluate(m_poseA);
    b.node->evaluate(m_poseB);

    for (size_t i = 0; i < outPose.size(); i++) {
        outPose[i] = m_poseA[i].interpolate(m_poseB[i], alpha);
    }
}