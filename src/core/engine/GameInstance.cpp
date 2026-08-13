#include "GameInstance.h"

#include <GL/glew.h>

#include <random>
#include <vector>

#include "AppConstants.h"
#include "Application.h"
#include "Logger.h"
#include "engine/assets/AssetManager.h"
#include "engine/assets/EngineAssets.h"
#include "engine/controllers/PlayerController.h"
#include "engine/env/lights/DirectionalLight.h"
#include "engine/env/lights/PointLight.h"
#include "engine/env/lights/Spotlight.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/lowlevelapi/TransformFeedbackShader.h"
#include "engine/rendering/mat/ChunkMaterial.h"
#include "engine/rendering/mat/LineMaterial.h"
#include "engine/rendering/mat/ParticleMaterial.h"
#include "engine/rendering/mat/SimpleMaterial.h"
#include "engine/rendering/mat/SkeletalMaterial.h"
#include "engine/rendering/mat/TransparentMaterial.h"
#include "foundation/threading/Future.h"
#include "rendering/StaticMesh.h"

GameInstance::GameInstance() : m_playerController(nullptr), m_player(nullptr), m_world(nullptr) {
    loadSettings(gameSettings);
}

GameInstance::~GameInstance() {
    saveSettings(gameSettings);
    deinitWorld();
}

void GameInstance::initializeWorld(World* newWorld) {
    if (m_world == nullptr) {
        gameState.deltaTime = 0.0f;
        gameState.elapsedGameTime = 0.0f;

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);
        m_playerController = new PlayerController;
        m_player = new Player;
        m_world = newWorld;
        m_world->setChunkLoadingDistance(gameSettings.graphics.renderDistance);
        m_playerController->possess(m_player);

        AudioEngine* audio = Application::getContext()->audioEngine;
        m_worldMusic = audio->playStreamed("res/audio/music/Deep Relaxation.mp3");
        audio->setBus(m_worldMusic, MUSIC_BUS);
        audio->setLooping(m_worldMusic, true);

        AssetManager* assets = Application::getContext()->assets;

        auto light1 = std::make_shared<PointLight>(glm::vec3(1.0f), 1.0f, 5.0f);
        light1->setCastsShadows(true);
        light1->setShadowPriority(1);
        light1->getLocalTransform().setPosition({1.5, 7, 0.5});
        light1->getLocalTransform().lookAt({0, 5, -4});
        m_lights.push_back(light1);

        for (int i = 0; i < 5; i++) {
            auto light2 = std::make_shared<Spotlight>(glm::vec3(1.0f), 1.0f, 45.0f, 10.0f);
            light2->setInnerCutoffAngle(25);
            light2->setCastsShadows(true);
            light2->setShadowPriority(1);
            light2->getLocalTransform().setPosition({15 + (i * 10), 8, 0});
            light2->getLocalTransform().lookAt({14 + (i * 10), 4, -4});
            m_lights.push_back(light2);
        }

        auto light3 = std::make_shared<DirectionalLight>(glm::vec3(1.0f, 0.86f, 0.25f), 0.5f);
        light3->setCastsShadows(true);
        light3->setShadowDistance(110);
        light3->setCascadeCount(3);
        light3->getLocalTransform().setPosition({15, 8, 0});
        light3->getLocalTransform().lookAt({14, 4, -4});
        m_lights.push_back(light3);

        Future<Shader> simpleShader = assets->request<Shader>(Assets::Shader::SIMPLE);
        Future<Shader> transparentShader = assets->request<Shader>(Assets::Shader::TRANSPARENT);
        Future<Texture> testBlockTexture = assets->request<Texture>(Assets::Texture::TESTBLOCK_TEXTURE);

        std::shared_ptr<Material> testMaterial1 = std::make_shared<SimpleMaterial>(
            simpleShader, transparentShader, glm::vec3(0.0f), testBlockTexture
        );
        std::shared_ptr<Material> testMaterial2 = std::make_shared<TransparentMaterial>(
            transparentShader, glm::vec4(0.5f, 0.5f, 0.0f, 0.8f)
        );
        std::shared_ptr<Material> testMaterial3 = std::make_shared<TransparentMaterial>(
            transparentShader, glm::vec4(0.2f, 0.1f, 0.7f, 0.4f)
        );
        Future<StaticMesh::Asset> testUnitBlockAsset = assets->request<StaticMesh::Asset>(
            Assets::Model::TEST_UNIT_BLOCK
        );
        m_mesh1 = std::make_shared<StaticMesh>(testUnitBlockAsset);
        m_mesh1->assignMaterial(testMaterial1);

        m_mesh2 = std::make_shared<StaticMesh>(testUnitBlockAsset);
        m_mesh2->assignMaterial(testMaterial2);

        m_mesh3 = std::make_shared<StaticMesh>(testUnitBlockAsset);
        m_mesh3->assignMaterial(testMaterial3);

        m_mesh1->getLocalTransform().setPosition(glm::vec3(0.0f, 10.0f, 0.0f));
        m_mesh1->getLocalTransform().setScale(1.0f);
        m_mesh1->attachChild(m_mesh2.get(), AttachRule::Full);
        m_mesh2->getLocalTransform().translate(glm::vec3(0.0f, 3.0f, 0.0f));

        m_mesh2->attachChild(m_mesh3.get(), AttachRule::Full);
        m_mesh3->getLocalTransform().translate(glm::vec3(0.0f, 1.0f, 1.0f));

        Future<Shader> lineShader = assets->request<Shader>(Assets::Shader::LINE);

        m_focusedBlockOutline = std::make_shared<Wireframe>(
            Wireframe::fromBoundigBox({glm::vec3(-0.005), glm::vec3(1.005)})
        );
        m_focusedBlockOutline->assignMaterial(std::make_shared<LineMaterial>(lineShader, glm::vec3(0.05, 0.05, 0.05)));
        m_focusedBlockOutline->setLineWidth(3.5f);

        Future<Shader> skeletalShader = assets->request<Shader>(Assets::Shader::SKELETAL_MESH);
        Future<Texture> skeletalTexture = assets->request<Texture>(Assets::Texture::TESTFLY_TEXTURE);
        Future<SkeletalMesh::Asset> skeletalMeshAsset = assets->request<SkeletalMesh::Asset>(Assets::Model::TESTFLY);
        m_skeletalMesh = std::make_shared<SkeletalMesh>(skeletalMeshAsset);
        m_skeletalMesh->assignMaterial(std::make_shared<SkeletalMaterial>(skeletalShader, skeletalTexture));
        m_skeletalMesh->getLocalTransform().setPosition(glm::vec3(10.0f, 8.0f, 5.0f));

        // Particles
        m_particles = std::make_shared<ParticleSystem>(std::vector<GenericGPUParticleModule>{
            ParticleModules::SpawnRate(50.0f),
            ParticleModules::SpawnBurst(3.0f, 200.0f),
            ParticleModules::SphereSpawn(0.5f),
            ParticleModules::Turbulence(50.0f),
            ParticleModules::InitialVelocityInCone(6.0f, 10.0f, glm::vec3(1.0, 2.0, 0.5), 15.0f),
            ParticleModules::Acceleration(glm::vec3(0, -9.86, 0)),
            ParticleModules::InitialLifetime(1.5f, 3.0f),
            ParticleModules::SizeOverLife({{0.5f, 1.0f}, {1.0f, 0.0f}}),
            ParticleModules::AnimatedTexture(4, 7, 3, 0.2f),
            ParticleModules::ColorOverLife(
                {{0.0f, glm::vec3(1, 1, 0.5)}, {0.5f, glm::vec3(0.5, 1, 0.5)}, {1.0f, glm::vec3(0, 0.5, 1)}}
            ),
        });
        Future<Texture> blockAtlasTexture = assets->request<Texture>(Assets::Texture::BLOCK_TEX_ATLAS);
        Future<TransformFeedbackShader> particleTfShader = assets->request<TransformFeedbackShader>(
            Assets::Shader::PARTICLE_TF
        );
        Future<Shader> particleShader = assets->request<Shader>(Assets::Shader::PARTICLE);
        m_particles->assignMaterial(
            std::make_shared<ParticleMaterial>(particleShader, particleTfShader, blockAtlasTexture)
        );
        m_particles->getLocalTransform().setPosition(glm::vec3(10.0f, 12.0f, 5.0f));
    }
}

void GameInstance::deinitWorld() {
    if (m_playerController) {
        delete m_playerController;
        m_playerController = nullptr;
    }
    if (m_player) {
        delete m_player;
        m_player = nullptr;
    }
    if (m_world) {
        try {
            Application::getContext()->audioEngine->stop(m_worldMusic);
            m_world->syncedSaveChunks();
        } catch (const std::exception& e) {
            lgr::lout.error(e.what());
        }
        delete m_world;
        m_world = nullptr;
    }
}

void GameInstance::pushWorldRenderData() {
    ApplicationContext* context = Application::getContext();

    Renderer* renderer = context->renderer;
    for (const auto& light : m_lights) {
        renderer->submitLight(light.get());
    }

    for (auto& val : m_world->loadedChunks()) {
        if (val.second.getMesh()) {
            renderer->submitRenderable(val.second.getMesh());
        }
    }

    renderer->submitRenderable(m_mesh1.get());
    renderer->submitRenderable(m_mesh2.get());
    renderer->submitRenderable(m_mesh3.get());
    renderer->submitRenderable(m_skeletalMesh.get());
    renderer->submitRenderable(m_particles.get());

    if (m_player->isFocusingBlock()) {
        m_focusedBlockOutline->getLocalTransform().setPosition(m_player->getFocusedBlock());
        renderer->submitRenderable(m_focusedBlockOutline.get());
    }
}

void GameInstance::update(float deltaTime) {
    gameState.deltaTime = deltaTime;
    gameState.elapsedGameTime += deltaTime;

    m_player->update(deltaTime);

    m_particles->update(deltaTime);

    Transform& mehs1Tr = m_mesh1->getLocalTransform();
    mehs1Tr.rotate(10.0f * deltaTime, WorldUp);
    m_mesh3->getLocalTransform().rotate(2.0f * deltaTime, WorldUp);
    m_world->updateChunks(m_player->getTransform().getPosition());

    if (!m_skeletalMesh->getActiveAnimation()) {
        m_skeletalMesh->playAnimation("Idle", true);
    }
    m_skeletalMesh->update(deltaTime);
}