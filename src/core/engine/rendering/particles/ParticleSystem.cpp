#include "ParticleSystem.h"

#include <GL/glew.h>

#include <cmath>  // For std::ceil / std::floor
#include <string>

#include "engine/rendering/GLUtils.h"
#include "engine/rendering/opengl/VertexBufferLayout.h"

#define MAX_PARTICLE_MODULES 100

static constexpr float particleQuad[] = {
    1.0f,  -1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f,  0.0f, 1.0f,

    1.0f,  -1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f,  0.0f, 1.0f,
    1.0f,  1.0f,  1.0f, 1.0f
};

ParticleSystem::ParticleSystem(const std::vector<GenericGPUParticleModule>& modules)
    : m_switched(false),
      m_accumulatedTime(0.0f),
      m_spawnAccumulator(0.0f),
      m_spawnRate(0.0f),
      m_spawnCount(0U),
      m_particleSpawnOffset(0U),
      m_newParticleSpawnOffset(0U),
      m_allocatedParticleCount(0U),
      m_flags(0U) {
    if (modules.size() > MAX_PARTICLE_MODULES)
        throw std::runtime_error(
            "More than" + std::to_string(MAX_PARTICLE_MODULES) + " modules were passed to particle system"
        );

    bool hasFixedParticleCount = false;
    bool hasSpawnRateModule = false;
    bool hasSpawnLocationModule = false;
    bool lifeTimeDefined = false;
    bool hasTextureModule = false;

    float maxLifetime = 0.0f;
    float totalBurstParticleCount = 0.0f;

    for (const GenericGPUParticleModule& module : modules) {
        if (module.flags & SPAWNMODULE_FLAG) {
            // Setup variable for spawning behaviour
            if (module.type == ModuleType::SpawnFixedParticleCount) {
                if (hasSpawnRateModule)
                    throw std::runtime_error(
                        "Cannot define SpawnFixedParticleCount when already having a dynamic spawn rate module"
                    );
                m_allocatedParticleCount = static_cast<size_t>(module.params[0].x);
                hasFixedParticleCount = true;
            } else if (module.type == ModuleType::SpawnRate) {
                if (hasFixedParticleCount)
                    throw std::runtime_error(
                        "Cannot define SpawnRate when already having SpawnFixedParticleCount module"
                    );
                m_spawnRate = module.params[0].x;
                m_flags |= DYNAMIC_SPAWNRATE;
                hasSpawnRateModule = true;
            } else if (module.type == ModuleType::SpawnBurst) {
                if (hasFixedParticleCount)
                    throw std::runtime_error(
                        "Cannot define SpawnBurst when already having SpawnFixedParticleCount module"
                    );
                m_burstSpawns.push_back({module.params[0].x, module.params[1].x, false});
                totalBurstParticleCount += module.params[1].x;
                m_flags |= DYNAMIC_SPAWNRATE;
                hasSpawnRateModule = true;
            }
        } else if (module.flags & SPAWNLOCATIONMODULE_FLAG) {
            if (hasSpawnLocationModule) throw std::runtime_error("Cannot define more than one spawn location module");
            hasSpawnLocationModule = true;
        } else if (module.flags & INITMODULE_FLAG) {
            if (module.type == ModuleType::InitialLifetime) {
                maxLifetime = module.params[1].x;
                lifeTimeDefined = true;
            } else if (module.type == ModuleType::InitialTexture) {
                if (hasTextureModule) {
                    throw std::runtime_error("Cannot define multiple texture modules");
                }
                m_flags |= USES_TEXTURE;
                hasTextureModule = true;
            }
        } else if (module.flags & UPDATEMODULE_FLAG) {
            if (module.type == ModuleType::AnimatedTexture) {
                if (hasTextureModule) {
                    throw std::runtime_error("Cannot define multiple texture modules");
                }
                m_flags |= USES_TEXTURE;
                hasTextureModule = true;
            }
        }
    }

    if (!hasFixedParticleCount && !hasSpawnRateModule)
        throw std::runtime_error("Must degine spawn module for particle system");
    if (!hasSpawnLocationModule) throw std::runtime_error("Must at least define one spawn location module");

    if (m_flags & DYNAMIC_SPAWNRATE) {
        float worstCaseActiveParticles = m_spawnRate * maxLifetime + totalBurstParticleCount;
        // Add safety margin. Otherwise accumulated spawns may not happen because particles are just barely alive.
        // Then it waits till the next accumulated spawn which causes spawn issues.
        size_t safetyMargin = std::max<size_t>(3, static_cast<size_t>(std::ceil(m_spawnRate * 0.05f)));
        m_allocatedParticleCount = static_cast<size_t>(std::ceil(worstCaseActiveParticles)) + safetyMargin;
    }

    m_tfFeedbackVAO1 = VertexArray::create();
    m_instanceDataVBO1 = VertexBuffer::create(nullptr, m_allocatedParticleCount * sizeof(Particle));
    m_instanceDataVBO1.clearData(); // Zero buffer memory

    m_tfFeedbackVAO2 = VertexArray::create();
    m_instanceDataVBO2 = VertexBuffer::create(nullptr, m_allocatedParticleCount * sizeof(Particle));
    m_instanceDataVBO2.clearData(); // Zero buffer memory

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 4);
    layout.push(GL_FLOAT, 3);
    layout.push(GL_FLOAT, 3);
    layout.push(GL_FLOAT, 1);
    layout.push(GL_FLOAT, 1);
    layout.push(GL_FLOAT, 1);
    layout.push(GL_UNSIGNED_INT, 1);

    m_instanceDataVBO1.setLayout(layout);
    m_tfFeedbackVAO1.addBuffer(m_instanceDataVBO1);

    m_instanceDataVBO2.setLayout(layout);
    m_tfFeedbackVAO2.addBuffer(m_instanceDataVBO2);

    m_verticesVBO = VertexBuffer::create(particleQuad, sizeof(particleQuad));

    VertexBufferLayout vLayout;
    vLayout.push(GL_FLOAT, 2);
    vLayout.push(GL_FLOAT, 2);
    m_verticesVBO.setLayout(vLayout);

    m_renderVAO1 = VertexArray::create();
    m_renderVAO1.addBuffer(m_verticesVBO);
    m_renderVAO1.addInstanceBuffer(m_instanceDataVBO1);

    m_renderVAO2 = VertexArray::create();
    m_renderVAO2.addBuffer(m_verticesVBO);
    m_renderVAO2.addInstanceBuffer(m_instanceDataVBO2);

    m_modulesUBO = UniformBuffer::create(modules.data(), modules.size() * sizeof(GenericGPUParticleModule));
}

void ParticleSystem::draw() const {
    if (m_switched) {
        m_renderVAO1.bind();
    } else {
        m_renderVAO2.bind();
    }
    GLCALL(glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_allocatedParticleCount));
}

void ParticleSystem::switchBuffers() { m_switched = !m_switched; }

void ParticleSystem::compute() {
    if (m_switched) {
        m_tfFeedbackVAO2.bind();
        GLCALL(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_instanceDataVBO1.rendererId()));
    } else {
        m_tfFeedbackVAO1.bind();
        GLCALL(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_instanceDataVBO2.rendererId()));
    }
    GLCALL(glBeginTransformFeedback(GL_POINTS));
    GLCALL(glDrawArrays(GL_POINTS, 0, m_allocatedParticleCount));
    GLCALL(glEndTransformFeedback());
}

void ParticleSystem::reset() {
    m_accumulatedTime = 0.0f;
    m_spawnAccumulator = 0.0f;
    for (BurstSpawn& bSpawn : m_burstSpawns) {
        bSpawn.fired = false;
    }
}

void ParticleSystem::update(float deltaTime) {
    if (m_flags & DYNAMIC_SPAWNRATE) {
        m_accumulatedTime += deltaTime;
        m_spawnAccumulator += deltaTime * m_spawnRate;

        for (BurstSpawn& bSpawn : m_burstSpawns) {
            if (!bSpawn.fired && bSpawn.delay < m_accumulatedTime) {
                m_spawnAccumulator += bSpawn.amount;
                bSpawn.fired = true;
            }
        }

        float spawnAmount = std::floor(m_spawnAccumulator);
        m_spawnAccumulator -= spawnAmount;
        m_spawnCount = static_cast<unsigned int>(spawnAmount);
        m_particleSpawnOffset = m_newParticleSpawnOffset;
        m_newParticleSpawnOffset = (m_particleSpawnOffset + m_spawnCount) % m_allocatedParticleCount;
    }
}