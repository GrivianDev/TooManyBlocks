#ifndef TOOMANYBLOCKS_APPLICATION_H
#define TOOMANYBLOCKS_APPLICATION_H

#include <stddef.h>

#include "foundation/util/SystemMetrics.h"
#include "platform/input/InputManager.h"
#include "platform/window/WindowManager.h"

struct ApplicationContext {
    struct AppState {
        unsigned int screenWidth;
        unsigned int screenHeight;
        int lastMousepositionX;
        int lastMousepositionY;
    } state;

    struct Stats {
        float cpuUsage;
        uint64_t processUsedBytes;
        MemoryInfo memInfo;
        ProcessIO processIo;
        CpuTimes cpuTimes;
    } stats;

    class Timer* timer;
    class ThreadPool* workerPool;
    class AssetManager* assets;
    class Renderer* renderer;
    class AudioEngine* audioEngine;
    class GameInstance* instance;
    class WindowManager* windowManager;
    class InputManager* inputManager;
};

class Application {
private:
    void updateStats(float deltaTime);

    void createContext();
    void deleteContext();

protected:
    static ApplicationContext* currentContext;

    virtual void init();
    virtual void execute();
    virtual void shutdown();

public:
    static ApplicationContext* getContext();

    void run();
};

#endif
