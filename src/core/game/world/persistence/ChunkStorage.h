#ifndef TOOMANYBLOCKS_CHUNKSTORAGE_H
#define TOOMANYBLOCKS_CHUNKSTORAGE_H

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "game/world/Chunk.h"

class ChunkStorage {
private:
    const std::filesystem::path m_chunkStoragePath;
    std::unordered_map<glm::ivec3, std::weak_ptr<std::mutex>, coord_hash> m_chunkLocks;
    std::mutex m_lockMapMutex;

    std::shared_ptr<std::mutex> getChunkMutex(const glm::ivec3& pos);

public:
    ChunkStorage(const std::filesystem::path& worldPath);

    bool hasChunk(const glm::ivec3& chunkPos);

    std::unique_ptr<Block[]> loadChunkData(const glm::ivec3& chunkPos);

    void saveChunkData(const glm::ivec3& chunkPos, const Block* blocks);
};

#endif