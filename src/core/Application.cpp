#include "Application.h"

#include <stdexcept>

#include "AppConstants.h"
#include "Logger.h"
#include "engine/GameInstance.h"
#include "engine/assets/AssetManager.h"
#include "engine/assets/AssetSetup.h"
#include "engine/rendering/Renderer.h"
#include "engine/ui/AboutScreen.h"
#include "engine/ui/GameOverlay.h"
#include "engine/ui/MainMenu.h"
#include "engine/ui/PauseMenu.h"
#include "engine/ui/SettingsMenu.h"
#include "engine/ui/Ui.h"
#include "engine/ui/WorldSelection.h"
#include "foundation/threading/Future.h"
#include "foundation/threading/ThreadPool.h"
#include "foundation/time/Timer.h"
#include "platform/audio/AudioEngine.h"
#include "platform/input/impl/GLFWInputManager.h"
#include "platform/window/impl/GLFWWindowManager.h"

#define WORKER_COUNT 6

ApplicationContext* Application::currentContext = nullptr;

static void scheduleCallback(std::unique_ptr<FutureBase> future, Executor executor) {
    ApplicationContext* context = Application::getContext();
    context->workerPool->pushJob(std::move(future), executor);
}

void Application::updateStats(float deltaTime) {
    ApplicationContext* context = Application::getContext();
    CpuTimes currentCpuTimes = getCpuTimes();
    context->stats.cpuUsage = getCpuUsage(context->stats.cpuTimes, currentCpuTimes, deltaTime);
    context->stats.memInfo = getSystemMemoryInfo();
    context->stats.processUsedBytes = getProcessUsedBytes();
    context->stats.processIo = getProcessIO();
    context->stats.cpuTimes = currentCpuTimes;
}

void Application::createContext() {
    if (Application::currentContext) {
        throw std::runtime_error("Application context already present");
    }

    ApplicationContext* context = new ApplicationContext{};

    context->timer = new Timer;
    context->workerPool = new ThreadPool(WORKER_COUNT);
    context->assets = new AssetManager;
    context->renderer = new Renderer;
    context->audioEngine = new AudioEngine;
    context->instance = new GameInstance;
    context->windowManager = new GLFWWindowManager;
    context->inputManager = new GLFWInputManager;

    Application::currentContext = context;
}

void Application::deleteContext() {
    if (ApplicationContext* context = Application::currentContext) {
        delete context->timer;
        delete context->assets;
        delete context->renderer;
        delete context->instance;
        delete context->audioEngine;
        delete context->workerPool;  // <- May clean up remaining opengl ressources here
        delete context->inputManager;
        delete context->windowManager;  // Keep last, destroys opengl context

        delete context;

        Application::currentContext = nullptr;
    }
}

void Application::init() {
    // Callback for scheduling framework
    FutureBase::scheduleCallback = scheduleCallback;

    // Create current context
    createContext();

    // Setup asset manager
    ApplicationContext* context = Application::getContext();

    registerAssetFactories(*context->assets);
    setupAssets(*context->assets);

    // Set initial screen dimensions

    context->state.screenWidth = 960;
    context->state.screenHeight = 540;

    context->windowManager->init();
    context->windowManager->createActiveWindow(960, 540, APP_NAME);
    context->windowManager->enableVSync(true);

    context->inputManager->setup();

    context->renderer->init();

    UI::init();

    UI::registerWidget<UI::MainMenu>("MainMenu");
    UI::registerWidget<UI::SettingsMenu>("SettingsMenu");
    UI::registerWidget<UI::GameOverlay>("GameOverlay");
    UI::registerWidget<UI::WorldSelection>("WorldSelection");
    UI::registerWidget<UI::PauseMenu>("PauseMenu");
    UI::registerWidget<UI::AboutScreen>("AboutScreen");
    UI::navigateTo("MainMenu");

    // Set initial output device
    if (context->audioEngine->loadDevices()) {
        int selectedDevice = -1;

        for (const AudioDevice& device : context->audioEngine->getOutputDevices()) {
            if (device.name == context->instance->gameSettings.audio.outputDevice) {
                selectedDevice = device.id;
                break;
            }
        }

        if (selectedDevice >= 0) {
            context->audioEngine->setOutputDevice(selectedDevice);
        } else {
            // Fall back to the default.
            for (const AudioDevice& device : context->audioEngine->getOutputDevices()) {
                if (device.isDefault) {
                    context->instance->gameSettings.audio.outputDevice = device.name;
                    context->audioEngine->setOutputDevice(device.id);
                    break;
                }
            }
        }
    }

    // Setup audio bus system
    context->audioEngine->setBusParent(MUSIC_BUS, MASTER_BUS);
    context->audioEngine->setBusParent(EFFECT_BUS, MASTER_BUS);
    context->audioEngine->setBusParent(AMBIENT_BUS, MASTER_BUS);

    context->audioEngine->setBusVolume(MASTER_BUS, context->instance->gameSettings.audio.masterVolume);
    context->audioEngine->setBusVolume(MUSIC_BUS, context->instance->gameSettings.audio.musicVolume);
    context->audioEngine->setBusVolume(EFFECT_BUS, context->instance->gameSettings.audio.effectVolume);
    context->audioEngine->setBusVolume(AMBIENT_BUS, context->instance->gameSettings.audio.ambientVolume);
}

void Application::execute() {
    ApplicationContext* context = Application::getContext();
    // Loop until the user closes the window
    try {
        float statUpdateAccumulator = 0.0f;

        while (!context->windowManager->shouldWindowClose() && !context->instance->gameState.quitGame) {
            context->timer->tick();

            statUpdateAccumulator += context->timer->deltaSeconds();
            if (statUpdateAccumulator >= 1.0f) {
                updateStats(statUpdateAccumulator);
                statUpdateAccumulator = 0.0f;
            }

            if (context->instance->isWorldInitialized()) {
                if (!context->instance->gameState.gamePaused) {
                    context->instance->update(context->timer->deltaSeconds());
                } else {
                    context->instance->gameState.deltaTime = 0.0f;
                }
                context->instance->pushWorldRenderData();
                context->renderer->render(*context);
            }

            context->assets->update(context->timer->deltaSeconds());
            context->audioEngine->update(context->timer->deltaSeconds());
            context->workerPool->processMainThreadJobs();
            context->workerPool->cleanupFinishedJobs();

            UI::render();

            // Swap front and back buffers
            context->windowManager->swapBuffers();
            // Poll for and process events, including set callbacks
            context->windowManager->pollEvents();
        }
    } catch (const std::exception& e) {
        lgr::lout.error("Error during game loop: " + std::string(e.what()));
    }
}

void Application::shutdown() {
    ApplicationContext* context = Application::getContext();
    context->workerPool->shutdown();

    UI::shutdown();

    deleteContext();
}

ApplicationContext* Application::getContext() {
    if (!Application::currentContext) throw std::runtime_error("Application context unavailable");
    return Application::currentContext;
}

void Application::run() {
    init();
    execute();
    shutdown();
}