#ifndef TOOMANYBLOCKS_ANIMATIONCLIP_H
#define TOOMANYBLOCKS_ANIMATIONCLIP_H

#include <memory>

#include "engine/animation/Timeline.h"
#include "engine/scene/Transform.h"

enum class AnimationProperty {
    Translation,
    Rotation,
    Scale
};

class AnimationClip {
public:
    struct Channel {
        int targetNodeIndex;
        AnimationProperty property;
        std::shared_ptr<TimelineBase> timeline;
    };

private:
    std::string m_name;
    std::vector<Channel> m_channels;
    float m_duration;

public:
    AnimationClip(const std::string& name, const std::vector<Channel>& channels);

    inline const std::string& getName() const { return m_name; };
    inline const std::vector<Channel>& getChannels() const { return m_channels; };
    inline float getDuration() const { return m_duration; };

    void sample(float time, std::vector<Transform>& outPose) const;
};

#endif
