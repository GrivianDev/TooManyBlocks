#include "AnimationClip.h"

AnimationClip::AnimationClip(const std::string& name, const std::vector<Channel>& channels)
    : m_name(name), m_channels(channels), m_duration(0.0f) {
    for (const Channel& c : m_channels) {
        m_duration = std::max<float>(m_duration, c.timeline->getEndTime());
    }
}

void AnimationClip::sample(float time, std::vector<Transform>& outPose) const {
    for (const Channel& channel : m_channels) {
        switch (channel.property) {
            case AnimationProperty::Translation: {
                auto timeline = std::static_pointer_cast<Timeline<glm::vec3>>(channel.timeline);
                outPose[channel.targetNodeIndex].setPosition(timeline->sample(time));
                break;
            }
            case AnimationProperty::Rotation: {
                auto timeline = std::static_pointer_cast<Timeline<glm::quat>>(channel.timeline);
                outPose[channel.targetNodeIndex].setRotation(timeline->sample(time));
                break;
            }
            case AnimationProperty::Scale: {
                auto timeline = std::static_pointer_cast<Timeline<float>>(channel.timeline);
                outPose[channel.targetNodeIndex].setScale(timeline->sample(time));
                break;
            }
        }
    }
}
