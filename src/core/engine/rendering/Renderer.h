#ifndef TOOMANYBLOCKS_RENDERER_H
#define TOOMANYBLOCKS_RENDERER_H

#include <array>
#include <memory>
#include <vector>

#include "compatability/Compatability.h"
#include "engine/env/lights/Light.h"
#include "engine/rendering/Renderable.h"
#include "engine/rendering/lowlevelapi/Texture.h"
#include "engine/rendering/lowlevelapi/UniformBuffer.h"
#include "engine/rendering/lowlevelapi/VertexArray.h"
#include "engine/rendering/lowlevelapi/VertexBuffer.h"
#include "engine/rendering/renderpasses/DebugReport.h"
#include "engine/rendering/renderpasses/Renderpass.h"

struct ApplicationContext;

struct LightingInfo {
    unsigned int activeLightsCount;
    std::array<Texture*, SHADOW_ATLAS_COUNT> shadowMapAtlases;
    const UniformBuffer* lightBuff;
    const UniformBuffer* shadowMapBuff;
};

struct TransformInfo {
    glm::mat4 viewProjection;
    glm::mat4 projection;
    glm::mat4 view;
    Transform viewportTransform;
    Transform meshTransform;
};

struct ParticleInfo {
    const UniformBuffer* pModulesBuff;
    unsigned int spawnCount;
    unsigned int particleSpawnOffset;
    unsigned int allocatedParticleCount;
    uint32_t flags;
};

struct SSAOInfo {
    bool enabled;
    const Texture* output;
};

struct OpaqueInfo {
    const Texture* output;
    std::shared_ptr<Texture> usedDepthTexture;
};

struct ResolverInfo {
    const Texture* output;
};

struct FXAAInfo {
    bool enabled;
    const Texture* output;
};

struct TransparencyInfo {
    const Texture* accumOutput;
    const Texture* revealOutput;
};

struct SkeletalMeshInfo {
    const UniformBuffer* jointMatrices;
};

struct RenderContext {
    glm::uvec2 currScreenRes;
    bool screenResChanged;
    float deltaTime;
    float elapsedTime;

    TransformInfo tInfo;
    SkeletalMeshInfo skInfo;
    LightingInfo lInfo;
    ParticleInfo pInfo;
    SSAOInfo ssaoInfo;
    OpaqueInfo opaqueInfo;
    TransparencyInfo transparencyInfo;
    ResolverInfo resolverInfo;
    FXAAInfo fxaaInfo;
};

struct RenderResources {
    const std::vector<Light*>* lightsToRender;
    const std::vector<Renderable*>* objectsToRender;

    std::vector<Light*> priodLightsBuffer;
    std::vector<Renderable*> culledObjectsBuffer;
};

class Renderer {
private:
    std::vector<Light*> m_lightsToRender;
    std::vector<Renderable*> m_objectsToRender;

    VertexArray m_fullScreenQuad_vao;
    VertexBuffer m_fullScreenQuad_vbo;

    RenderContext m_currentRenderContext;
    RenderResources m_renderResources;
    std::vector<std::unique_ptr<Renderpass>> m_renderpasses;

    int m_lastLightCount;
    int m_lastObjectCount;
    float m_lastRenderTimeMs;

public:
    Renderer() : m_currentRenderContext{}, m_renderResources{} {}

    void init();

    void submitLight(Light* light);

    void submitRenderable(Renderable* obj);

    void render(const ApplicationContext& context);

    void drawFullscreenQuad();

    void fillDebugReport(DebugReport& report) const;

    void setDebugPolygonModeEnabled(bool enabled);

    template <typename T>
    T* getPass() {
        for (const std::unique_ptr<Renderpass>& pass : m_renderpasses) {
            if (T* typedPass = dynamic_cast<T*>(pass.get())) return typedPass;
        }

        return nullptr;
    }
};

#endif
