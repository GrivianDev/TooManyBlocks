#ifndef TOOMANYBLOCKS_WORLD_H
#define TOOMANYBLOCKS_WORLD_H

#include <filesystem>
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "engine/Updatable.h"
#include "engine/assets/cpu/CPURenderData.h"
#include "engine/rendering/material/ChunkMaterial.h"
#include "engine/scene/Scene.h"
#include "foundation/threading/Future.h"
#include "game/blocks/BlockToTextureMapping.h"
#include "game/world/Chunk.h"
#include "game/world/persistence/ChunkStorage.h"

class World : public Updatable {
private:
    uint64_t m_taskContext;
    Scene m_scene;

    uint32_t m_seed;
    const std::filesystem::path m_worldDir;
    ChunkStorage m_cStorage;
    int chunkLoadingDistance;
    std::unordered_map<glm::ivec3, Chunk, coord_hash> m_loadedChunks;
    std::shared_ptr<Material> m_chunkMaterial;

    std::unordered_map<glm::ivec3, uint16_t, coord_hash> m_pendingChanges;

    std::unordered_set<glm::ivec3, coord_hash> determineActiveChunks(const glm::ivec3& position);

public:
    const BlockToTextureMap texMap;

    World(const std::filesystem::path& worldDir);

    ~World();

    inline uint32_t seed() const { return m_seed; }

    inline Scene& scene() { return m_scene; }

    Chunk* getChunk(const glm::ivec3& location);

    void updateChunks(const glm::ivec3& position);

    void syncedSaveChunks();

    void setBlock(const glm::ivec3& position, uint16_t newBlocks);

    inline std::unordered_map<glm::ivec3, Chunk, coord_hash>& loadedChunks() { return m_loadedChunks; }

    inline void setChunkLoadingDistance(int distance) { chunkLoadingDistance = distance; }

    inline int getChunkLoadingDistance() const { return chunkLoadingDistance; }

    void update(float deltaTime) override;
};

#endif
