#ifndef TOOMANYBLOCKS_TIMER_H
#define TOOMANYBLOCKS_TIMER_H

#include <cstdint>

class Timer {
private:
    double m_previousSeconds;
    double m_elapsedSeconds;
    double m_deltaSeconds;
    uint64_t m_frame;

    double m_frameStartSeconds;

public:
    Timer();
    void tick();

    // FPS limiting
    void startFrame();
    void limitFPS(double fps);

    inline double elapsedSeconds() const { return m_elapsedSeconds; };
    inline double deltaSeconds() const { return m_deltaSeconds; };
    inline uint64_t frameIndex() const { return m_frame; };
};

#endif
