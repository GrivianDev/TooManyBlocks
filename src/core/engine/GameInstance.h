#ifndef TOOMANYBLOCKS_GAMEINSTANCE_H
#define TOOMANYBLOCKS_GAMEINSTANCE_H

#include "Updatable.h"
#include "engine/GameSettings.h"
#include "engine/entity/Player.h"
#include "engine/env/World.h"
#include "platform/audio/AudioEngine.h"

struct GameState {
    float elapsedGameTime = 0.0f;
    float deltaTime = 0.0f;
    bool gamePaused = false;
    bool quitGame = false;
};

class GameInstance : public Updatable {
public:
    GameState gameState;
    GameSettings gameSettings;
    Controller* m_playerController;
    Player* m_player;
    AudioInstance m_worldMusic;
    World* m_world;

public:
    GameInstance();
    virtual ~GameInstance();

    void initializeWorld(World* newWorld);

    void deinitWorld();

    inline bool isWorldInitialized() const { return m_world != nullptr; }

    void pushWorldRenderData();

    void update(float deltaTime) override;
};

#endif
