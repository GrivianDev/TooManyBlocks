#include "Timer.h"

#include <chrono>
#include <thread>

using Clock = std::chrono::steady_clock;

static const Clock::time_point startTime = Clock::now();

Timer::Timer() : m_previousSeconds(0), m_elapsedSeconds(0), m_deltaSeconds(0), m_frame(0), m_frameStartSeconds(0) {}

void Timer::tick() {
    double now = std::chrono::duration<double>(Clock::now() - startTime).count();
    if (m_frame == 0) {  // Avoid first frame spike
        m_previousSeconds = now;
    }

    m_elapsedSeconds = now;
    m_deltaSeconds = m_elapsedSeconds - m_previousSeconds;
    m_previousSeconds = m_elapsedSeconds;
    m_frame++;
}

void Timer::startFrame() { m_frameStartSeconds = std::chrono::duration<double>(Clock::now() - startTime).count(); }

void Timer::limitFPS(double fps) {
    if (fps <= 0.0) {
        return;
    }

    const double targetFrameTime = 1.0 / fps;

    double now = std::chrono::duration<double>(Clock::now() - startTime).count();

    double frameTime = now - m_frameStartSeconds;
    double remaining = targetFrameTime - frameTime;

    if (remaining > 0.0) {
        std::this_thread::sleep_for(std::chrono::duration<double>(remaining));
    }
}
