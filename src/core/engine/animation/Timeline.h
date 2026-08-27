#ifndef TOOMANYBLOCKS_TIMELINE_H
#define TOOMANYBLOCKS_TIMELINE_H

#include <algorithm>
#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <vector>

enum class Interpolation {
    STEP,
    LINEAR
    // TODO: Add CubicSpline later
};

template <typename T>
struct Keyframe {
    float time;
    T value;
};

template <typename T>
struct InterpolationPolicy {
    static T Linear(const T& a, const T& b, float t) { return a * (1.0f - t) + b * t; }
};

template <>
struct InterpolationPolicy<glm::quat> {
    static glm::quat Linear(const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); }
};

class TimelineBase {
public:
    virtual ~TimelineBase() = default;

    virtual float getStartTime() const = 0;
    virtual float getEndTime() const = 0;
};

template <typename T>
class Timeline : public TimelineBase {
private:
    std::vector<Keyframe<T>> m_keyframes;
    Interpolation m_interpMode;

    int findLastBeforeOrEqual(float time) const {
        int left = 0;
        int right = m_keyframes.size() - 1;
        int result = -1;

        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (m_keyframes[mid].time <= time) {
                result = mid;  // valid candidate, but maybe theres a better one to the right
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        // index of the best candidate, or -1 if none match
        return result;
    }

public:
    Timeline(
        const std::vector<Keyframe<T>>& keyframes,
        Interpolation interpMode = Interpolation::LINEAR
    )
        : m_keyframes(keyframes), m_interpMode(interpMode) {
        std::sort(m_keyframes.begin(), m_keyframes.end(), [](const Keyframe<T>& a, const Keyframe<T>& b) {
            return a.time < b.time;
        });
    }

    Timeline(
        std::vector<Keyframe<T>>&& keyframes,
        Interpolation interpMode = Interpolation::LINEAR
    )
        : m_keyframes(std::move(keyframes)), m_interpMode(interpMode) {
        std::sort(m_keyframes.begin(), m_keyframes.end(), [](const Keyframe<T>& a, const Keyframe<T>& b) {
            return a.time < b.time;
        });
    }

    T sample(float time) const {
        if (m_keyframes.empty()) {
            throw std::runtime_error("No keyframes to sample from!");
        }

        // Clamp to first or last
        if (time <= m_keyframes.front().time) return m_keyframes.front().value;
        if (time >= m_keyframes.back().time) return m_keyframes.back().value;

        // Binary search for the right interval
        int index = findLastBeforeOrEqual(time);

        const Keyframe<T>& kf0 = m_keyframes[index];
        const Keyframe<T>& kf1 = m_keyframes[index + 1];

        float t = (time - kf0.time) / (kf1.time - kf0.time);

        switch (m_interpMode) {
            case Interpolation::STEP: return kf0.value;  // Return first frame
            case Interpolation::LINEAR:
            default: return InterpolationPolicy<T>::Linear(kf0.value, kf1.value, t);
        }
    }

    inline void setInterpolationMode(Interpolation interpMode) { m_interpMode = interpMode; }

    inline Interpolation getInterpolationMode() const { return m_interpMode; }

    inline const std::vector<Keyframe<T>>& getKeyframes() const {
        return m_keyframes;
    }

    inline float getStartTime() const override {
        if (m_keyframes.empty()) return 0.0f;
        return m_keyframes.front().time;
    }

    inline float getEndTime() const override {
        if (m_keyframes.empty()) return 0.0f;
        return m_keyframes.back().time;
    }
};

#endif