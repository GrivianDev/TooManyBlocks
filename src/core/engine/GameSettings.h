#ifndef TOOMANYBLOCKS_GAMESETTINGS_H
#define TOOMANYBLOCKS_GAMESETTINGS_H

#include <string>

constexpr unsigned int MASTER_BUS = 0;
constexpr unsigned int MUSIC_BUS = 1;
constexpr unsigned int EFFECT_BUS = 2;
constexpr unsigned int AMBIENT_BUS = 3;

struct GraphicsSettings {
    // Window
    int windowMode = 0;
    bool vsync = true;

    // Performance
    int maxFramerate = 0;  // 0 = unlimited

    // Rendering
    int renderDistance = 16;
    int simulationDistance = 12;

    float renderScale = 1.0f;
    float fieldOfView = 70.0f;

    bool entityShadows = true;
    bool particles = true;
    bool clouds = true;
    bool ambientOcclusion = true;

    // Quality
    int shadowQuality = 2;
    int textureQuality = 2;
    int particleQuality = 2;

    // Debug
    bool debugPolygonModeEnabled = false;
};

struct AudioSettings {
    std::string outputDevice;

    float masterVolume = 1.0f;
    float musicVolume = 0.8f;
    float effectVolume = 1.0f;
    float ambientVolume = 1.0f;
};

struct InputSettings {
    float mouseSensitivity = 1.0f;
    bool invertMouseY = false;
    bool rawMouseInput = true;

    struct KeyBindings {
        // TODO How to model this?
    } keys;
};

struct GameplaySettings {
    // HUD
    bool showHud = true;
    bool showCrosshair = true;
    bool showHotbar = true;

    // Gameplay
    bool autoJump = false;
    bool tutorialHints = true;
    bool showBlockOutline = true;

    // Camera
    bool viewBobbing = true;

    // Interaction
    bool autoTool = false;
};

struct InterfaceSettings {
    float uiScale = 1.0f;

    bool showFps = false;

    bool pauseOnLostFocus = true;
};

struct GameSettings {
    GraphicsSettings graphics;
    AudioSettings audio;
    InputSettings input;
    GameplaySettings gameplay;
    InterfaceSettings interface;
};

void loadSettings(GameSettings& settings);

void saveSettings(const GameSettings& settings);

#endif
