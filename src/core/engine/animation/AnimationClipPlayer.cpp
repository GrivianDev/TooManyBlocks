#include "AnimationClipPlayer.h"

AnimationClipPlayer::AnimationClipPlayer()
    : m_clip(nullptr), m_time(0.0f), m_speed(1.0f), m_looping(false), m_finished(false), m_paused(false) {}

void AnimationClipPlayer::play(const AnimationClip* clip, bool loop, bool restart) {
    if (!clip) {
        stop();
        return;
    }

    bool sameClip = (m_clip == clip);

    m_clip = clip;
    m_looping = loop;
    m_finished = false;
    m_paused = false;

    if (!sameClip || restart) m_time = 0.0f;
}

void AnimationClipPlayer::resume() {
    if (m_clip && m_paused && !m_finished) m_paused = false;
}

void AnimationClipPlayer::pause() {
    if (m_clip && !m_finished) m_paused = true;
}

void AnimationClipPlayer::stop() {
    m_clip = nullptr;
    m_time = 0.0f;
    m_finished = false;
    m_paused = false;
}

void AnimationClipPlayer::setTime(float time) {
    if (!m_clip) {
        m_time = 0.0f;
        return;
    }

    m_time = std::clamp(time, 0.0f, m_clip->getDuration());

    m_finished = !m_looping && m_time >= m_clip->getDuration();
}

float AnimationClipPlayer::getNormalizedTime() const {
    if (!m_clip || m_clip->getDuration() <= 0.0f) return 0.0f;
    return std::clamp(m_time / m_clip->getDuration(), 0.0f, 1.0f);
}

void AnimationClipPlayer::update(float deltaTime) {
    if (!m_clip || m_finished || m_paused) return;

    float duration = m_clip->getDuration();
    if (duration <= 0.0f) {
        m_time = 0.0f;
        m_finished = true;
        return;
    }

    m_time += m_speed * deltaTime;

    if (m_looping) {
        m_time = std::fmod(m_time, duration);
        if (m_time < 0.0f) m_time += duration;
    } else {
        if (m_time >= duration) {
            m_time = duration;
            m_finished = true;
        } else if (m_time < 0.0f) {
            m_time = 0.0f;
            m_finished = true;
        }
    }
}

void AnimationClipPlayer::evaluate(std::vector<Transform>& outPose) {
    if (!m_clip) return;
    m_clip->sample(m_time, outPose);
}
