#ifndef TOOMANYBLOCKS_ANIMATIONCLIPPLAYER_H
#define TOOMANYBLOCKS_ANIMATIONCLIPPLAYER_H

#include "engine/animation/AnimationClip.h"
#include "engine/animation/AnimationNode.h"

class AnimationClipPlayer : public AnimationNode {
private:
    const AnimationClip* m_clip;

    float m_time;
    float m_speed;

    bool m_looping;
    bool m_finished;
    bool m_paused;

public:
    AnimationClipPlayer();
    virtual ~AnimationClipPlayer() = default;

    void play(const AnimationClip* clip, bool loop = false, bool restart = true);
    void resume();
    void pause();
    void stop();

    inline void setSpeed(float speed) { m_speed = speed; };
    void setTime(float time);

    inline float getSpeed() const { return m_speed; }
    inline float getTime() const { return m_time; };
    float getNormalizedTime() const;

    inline bool isPlaying() const { return m_clip && !m_finished && !m_paused; }
    inline bool isPaused() const { return m_clip && !m_paused; }
    inline bool isFinished() const { return m_finished; }

    void update(float deltaTime) override;

    void evaluate(std::vector<Transform>& outPose) override;
};

#endif
