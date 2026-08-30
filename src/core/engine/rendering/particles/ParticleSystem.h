#ifndef TOOMANYBLOCKS_PARTICLESYSTEM_H
#define TOOMANYBLOCKS_PARTICLESYSTEM_H

#include <glm/glm.hpp>

#include "engine/Updatable.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/opengl/UniformBuffer.h"
#include "engine/rendering/opengl/VertexArray.h"
#include "engine/rendering/opengl/VertexBuffer.h"
#include "engine/rendering/particles/ParticleModules.h"
#include "engine/scene/renderables/Renderable.h"

struct Particle {
    glm::vec4 color;
    glm::vec3 position;
    glm::vec3 velocity;
    float timeToLive;
    float initialTimeToLive;
    float size;
    uint32_t metadata;
};

class ParticleSystem : public Renderable, public Updatable {
private:
    struct BurstSpawn {
        float delay;
        float amount;
        bool fired;
    };

    VertexArray m_tfFeedbackVAO1;
    VertexArray m_tfFeedbackVAO2;
    VertexBuffer m_instanceDataVBO1;
    VertexBuffer m_instanceDataVBO2;

    VertexArray m_renderVAO1;
    VertexArray m_renderVAO2;
    VertexBuffer m_verticesVBO;

    UniformBuffer m_modulesUBO;

    bool m_switched;

    float m_accumulatedTime;
    float m_spawnAccumulator;
    float m_spawnRate;
    std::vector<BurstSpawn> m_burstSpawns;

    unsigned int m_spawnCount;
    unsigned int m_particleSpawnOffset;
    unsigned int m_newParticleSpawnOffset;
    unsigned int m_allocatedParticleCount;
    uint32_t m_flags;

public:
    ParticleSystem(const std::vector<GenericGPUParticleModule>& modules);
    virtual ~ParticleSystem() = default;

    virtual void draw() const override;

    void switchBuffers();

    void compute();

    void reset();

    void update(float deltaTime) override;

    const UniformBuffer* getModulesUBO() const { return &m_modulesUBO; }

    virtual BoundingBox getBoundingBox() const override { return BoundingBox::notCullable(); };

    unsigned int getSpawnCount() const { return m_spawnCount; }

    unsigned int getParticleSpawnOffset() const { return m_particleSpawnOffset; }

    unsigned int getAllocatedParticleCount() const { return m_allocatedParticleCount; }

    uint32_t getFlags() const { return m_flags; }
};

#endif
