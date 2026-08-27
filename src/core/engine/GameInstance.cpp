#include "GameInstance.h"

#include <GL/glew.h>

#include <memory>

#include "AppConstants.h"
#include "Application.h"
#include "Logger.h"
#include "engine/animation/AnimationClipPlayer.h"
#include "engine/animation/AnimationStateMachine.h"
#include "engine/animation/BlendSpacePlayer1D.h"
#include "engine/animation/BoneMask.h"
#include "engine/assets/AssetManager.h"
#include "engine/assets/EngineAssets.h"
#include "engine/controllers/PlayerController.h"
#include "engine/env/lights/DirectionalLight.h"
#include "engine/env/lights/PointLight.h"
#include "engine/env/lights/Spotlight.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/SkeletalMesh.h"
#include "engine/rendering/StaticMesh.h"
#include "engine/rendering/Wireframe.h"
#include "engine/rendering/lowlevelapi/Shader.h"
#include "engine/rendering/lowlevelapi/Texture.h"
#include "engine/rendering/lowlevelapi/TransformFeedbackShader.h"
#include "engine/rendering/mat/ChunkMaterial.h"
#include "engine/rendering/mat/LineMaterial.h"
#include "engine/rendering/mat/ParticleMaterial.h"
#include "engine/rendering/mat/SimpleMaterial.h"
#include "engine/rendering/mat/SkeletalMaterial.h"
#include "engine/rendering/mat/TransparentMaterial.h"
#include "engine/rendering/particles/ParticleSystem.h"
#include "foundation/threading/Future.h"

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

        m_playerController = new PlayerController;
        m_player = new Player;
        m_world = newWorld;
        m_world->setChunkLoadingDistance(gameSettings.graphics.renderDistance);
        m_playerController->possess(m_player);

        Scene& scene = m_world->scene();

        AudioEngine* audio = Application::getContext()->audioEngine;
        m_worldMusic = audio->playStreamed("res/audio/music/Deep Relaxation.mp3");
        audio->setBus(m_worldMusic, MUSIC_BUS);
        audio->setLooping(m_worldMusic, true);

        AssetManager* assets = Application::getContext()->assets;

        PointLight* pointLight = scene.create<PointLight>(glm::vec3(1.0f), 1.0f, 5.0f);
        pointLight->setCastsShadows(true);
        pointLight->setShadowPriority(1);
        pointLight->getLocalTransform().setPosition({1.5, 7, 0.5});
        pointLight->getLocalTransform().lookAt({0, 5, -4});

        for (int i = 0; i < 5; i++) {
            Spotlight* spotLight = scene.create<Spotlight>(glm::vec3(1.0f), 1.0f, 45.0f, 10.0f);
            spotLight->setInnerCutoffAngle(25);
            spotLight->setCastsShadows(true);
            spotLight->setShadowPriority(1);
            spotLight->getLocalTransform().setPosition({15 + (i * 10), 8, 0});
            spotLight->getLocalTransform().lookAt({14 + (i * 10), 4, -4});
        }

        DirectionalLight* directionalLight = scene.create<DirectionalLight>(glm::vec3(1.0f, 0.86f, 0.25f), 0.5f);
        directionalLight->setCastsShadows(true);
        directionalLight->setShadowDistance(110);
        directionalLight->setCascadeCount(4);
        directionalLight->getLocalTransform().setPosition({15, 8, 0});
        directionalLight->getLocalTransform().lookAt({14, 4, -4});

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
        StaticMesh* mesh1 = scene.create<StaticMesh>(testUnitBlockAsset, testMaterial1);
        mesh1->setName("MyTestRootBlock1");
        StaticMesh* mesh2 = scene.create<StaticMesh>(testUnitBlockAsset, testMaterial2);
        mesh2->setName("MyTestRootBlock2");
        StaticMesh* mesh3 = scene.create<StaticMesh>(testUnitBlockAsset, testMaterial3);
        mesh3->setName("MyTestRootBlock3");

        mesh1->getLocalTransform().setPosition(glm::vec3(0.0f, 10.0f, 0.0f));
        mesh1->getLocalTransform().setScale(1.0f);
        mesh1->attachChild(mesh2, AttachRule::Full);
        mesh2->getLocalTransform().translate(glm::vec3(0.0f, 3.0f, 0.0f));

        mesh2->attachChild(mesh3, AttachRule::Full);
        mesh3->getLocalTransform().translate(glm::vec3(0.0f, 1.0f, 1.0f));

        Future<Shader> lineShader = assets->request<Shader>(Assets::Shader::LINE);

        Wireframe* focusedBlockOutline = scene.create<Wireframe>(
            Wireframe::fromBoundigBox({glm::vec3(-0.005), glm::vec3(1.005)})
        );
        focusedBlockOutline->addTag("FocusBlockOutline");
        focusedBlockOutline->assignMaterial(std::make_shared<LineMaterial>(lineShader, glm::vec3(0.05, 0.05, 0.05)));
        focusedBlockOutline->setLineWidth(3.5f);

        Future<Shader> skeletalShader = assets->request<Shader>(Assets::Shader::SKELETAL_MESH);
        Future<Texture> skeletalTexture = assets->request<Texture>(Assets::Texture::HUMANOID_TEXTURE);
        Future<SkeletalMesh::Asset> skeletalMeshAsset = assets->request<SkeletalMesh::Asset>(Assets::Model::HUMANOID);
        SkeletalMesh* skeletalMesh = scene.create<SkeletalMesh>(
            skeletalMeshAsset, std::make_shared<SkeletalMaterial>(skeletalShader, skeletalTexture)
        );
        skeletalMesh->setName("MySkeletalMesh");
        skeletalMesh->getLocalTransform().setPosition(glm::vec3(6.0f, 8.0f, 5.0f));
        skeletalMesh->getLocalTransform().setScale(0.2f);

        // Particles
        ParticleSystem* particles = scene.create<ParticleSystem>(std::vector<GenericGPUParticleModule>{
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
        particles->assignMaterial(
            std::make_shared<ParticleMaterial>(particleShader, particleTfShader, blockAtlasTexture)
        );
        particles->getLocalTransform().setPosition(glm::vec3(10.0f, 12.0f, 5.0f));
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
    for (Light* light : m_world->scene().getLights()) {
        context->renderer->submitLight(light);
    }
    for (Renderable* sceneObject : m_world->scene().getRenderables()) {
        context->renderer->submitRenderable(sceneObject);
    }
}

void GameInstance::update(float deltaTime) {
    // A bunch of debug / showcase stuff
    static float accumulatorToggle = 0.0f;
    accumulatorToggle += deltaTime;

    static float blendDirection = -1.0f;

    gameState.deltaTime = deltaTime;
    gameState.elapsedGameTime += deltaTime;

    m_player->update(deltaTime);

    if (SceneComponent* mesh1 = m_world->scene().findByName("MyTestRootBlock1")) {
        mesh1->getLocalTransform().rotate(10.0f * deltaTime, WorldUp);
    }
    if (SceneComponent* mesh3 = m_world->scene().findByName("MyTestRootBlock3")) {
        mesh3->getLocalTransform().rotate(2.0f * deltaTime, WorldUp);
    }

    m_world->updateChunks(m_player->getTransform().getPosition());
    m_world->update(deltaTime);

    SkeletalMesh* skeletalMesh = static_cast<SkeletalMesh*>(m_world->scene().findByName("MySkeletalMesh"));
    if (skeletalMesh && accumulatorToggle > 5.0f) {
        if (skeletalMesh->hasTag("ShouldRun")) {
            skeletalMesh->removeTag("ShouldRun");

        } else {
            skeletalMesh->addTag("ShouldRun");
        }
        accumulatorToggle = 0.0f;
    }
    if (skeletalMesh) {
        skeletalMesh->testValue += blendDirection * deltaTime * 0.1f;

        if (skeletalMesh->testValue >= 1.0f) {
            skeletalMesh->testValue = 1.0f;
            blendDirection = -1.0f;
        } else if (skeletalMesh->testValue <= 0.0f) {
            skeletalMesh->testValue = 0.0f;
            blendDirection = 1.0f;
        }
    }

    if (skeletalMesh->isReady() && !skeletalMesh->getAnimationController()) {
        std::unique_ptr<AnimationController> controller = std::make_unique<AnimationController>(skeletalMesh);

        std::unique_ptr<AnimationClipPlayer> idlePlayer = std::make_unique<AnimationClipPlayer>();
        idlePlayer->play(skeletalMesh->getAnimation("Idle"), true);

        std::unique_ptr<AnimationClipPlayer> walkPlayer = std::make_unique<AnimationClipPlayer>();
        walkPlayer->play(skeletalMesh->getAnimation("Walk"), true);

        std::unique_ptr<AnimationClipPlayer> runPlayer = std::make_unique<AnimationClipPlayer>();
        runPlayer->play(skeletalMesh->getAnimation("Run"), true);

        // Blendspace
        std::unique_ptr<BlendSpacePlayer1D> blendSpace = std::make_unique<BlendSpacePlayer1D>(
            skeletalMesh, [](const SkeletalMesh* owner) { return owner->testValue; }
        );
        blendSpace->setSpeed(2.0f);
        blendSpace->addSample(0.0f, std::move(idlePlayer));
        blendSpace->addSample(0.5f, std::move(walkPlayer));
        blendSpace->addSample(1.0f, std::move(runPlayer));

        std::unique_ptr<AnimationClipPlayer> idlePlayer2 = std::make_unique<AnimationClipPlayer>();
        idlePlayer2->play(skeletalMesh->getAnimation("Idle"), true);
        std::unique_ptr<AnimationClipPlayer> runPlayer2 = std::make_unique<AnimationClipPlayer>();
        runPlayer2->play(skeletalMesh->getAnimation("Run"), true);

        // State machine
        std::unique_ptr<AnimationStateMachine> stateMachine = std::make_unique<AnimationStateMachine>(skeletalMesh);
        stateMachine->addState("IdleState", std::move(idlePlayer2));
        stateMachine->addState("RunState", std::move(runPlayer2));

        stateMachine->addTransition(
            "IdleState", "RunState", [](const SkeletalMesh* owner) { return owner->hasTag("ShouldRun"); }, 2.0f
        );
        stateMachine->addTransition(
            "RunState", "IdleState", [](const SkeletalMesh* owner) { return !owner->hasTag("ShouldRun"); }, 0.5f
        );
        stateMachine->transitionTo("IdleState");

        AnimationLayer& layer1 = controller->addLayer(std::move(stateMachine));
        BoneMask torsoMask = BoneMask::hierarchy(skeletalMesh->getAssetHandle().value().nodeArray, "Torso");
        layer1.setMask(torsoMask);

        AnimationLayer& layer2 = controller->addLayer(std::move(blendSpace));
        layer2.setMask(~torsoMask);

        skeletalMesh->setAnimationController(std::move(controller));
    }
    SceneComponent* blockOutline = m_world->scene().findByTag("FocusBlockOutline");
    blockOutline->setVisible(m_player->isFocusingBlock());
    blockOutline->getLocalTransform().setPosition(m_player->getFocusedBlock());
}