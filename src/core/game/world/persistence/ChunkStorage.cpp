#include "ChunkStorage.h"

#include <climits>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

#include "game/blocks/BlockTypes.h"

static std::string chunkFileNameFromCoords(const glm::ivec3& chunkPos) {
    return "chunk_" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + "_" + std::to_string(chunkPos.z) +
           ".chnk";
}

std::shared_ptr<std::mutex> ChunkStorage::getChunkMutex(const glm::ivec3& pos) {
    std::lock_guard<std::mutex> lock(m_lockMapMutex);

    // Clean up expired entries
    for (auto it = m_chunkLocks.begin(); it != m_chunkLocks.end();) {
        if (it->second.expired()) {
            it = m_chunkLocks.erase(it);
        } else {
            ++it;
        }
    }

    // Get or create the mutex for this chunk
    auto it = m_chunkLocks.find(pos);
    if (it != m_chunkLocks.end()) {
        if (auto existing = it->second.lock()) {
            return existing;
        }
    }

    std::shared_ptr<std::mutex> newMutex = std::make_shared<std::mutex>();
    m_chunkLocks[pos] = newMutex;
    return newMutex;
}

ChunkStorage::ChunkStorage(const std::filesystem::path& worldPath) : m_chunkStoragePath(worldPath / "chunks") {
    if (!std::filesystem::exists(m_chunkStoragePath)) {
        std::filesystem::create_directories(m_chunkStoragePath);
    }
}

bool ChunkStorage::hasChunk(const glm::ivec3& chunkPos) {
    std::shared_ptr<std::mutex> fileAccessMutex = getChunkMutex(chunkPos);
    std::lock_guard<std::mutex> lock(*fileAccessMutex);

    std::string chunkFileName = chunkFileNameFromCoords(chunkPos);
    return std::filesystem::exists(m_chunkStoragePath / chunkFileName);
}

std::unique_ptr<Block[]> ChunkStorage::loadChunkData(const glm::ivec3& chunkPos) {
    std::shared_ptr<std::mutex> fileAccessMutex = getChunkMutex(chunkPos);
    std::lock_guard<std::mutex> lock(*fileAccessMutex);

    std::string chunkFileName = chunkFileNameFromCoords(chunkPos);
    std::filesystem::path chunkFilePath = m_chunkStoragePath / chunkFileName;

    std::ifstream file(chunkFilePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open chunk file for read: " + chunkFilePath.string());
    }

    // Read 4-byte header
    char header[4];
    file.read(header, 4);
    if (file.gcount() != 4) {
        throw std::runtime_error("Invalid chunk file: " + chunkFilePath.string());
    }
    if (std::strncmp(header, "CHNK", 4) != 0) {
        throw std::runtime_error("Unexpected header in chunk file: " + chunkFilePath.string());
    }

    // Read 1-byte version
    uint8_t version = 0;
    file.read(reinterpret_cast<char*>(&version), 1);
    if (file.gcount() != 1) {
        throw std::runtime_error("Failed to read version byte from chunk file: " + chunkFilePath.string());
    }

    // Read 1-byte flags
    uint8_t flags = 0;
    file.read(reinterpret_cast<char*>(&flags), 1);
    if (file.gcount() != 1) {
        throw std::runtime_error("Failed to read flags byte from chunk file: " + chunkFilePath.string());
    }

    size_t blockIndex = 0;
    std::unique_ptr<Block[]> blocks(new Block[BLOCKS_PER_CHUNK], std::default_delete<Block[]>());

    while (file && blockIndex < BLOCKS_PER_CHUNK) {
        uint16_t rlePair[2];

        file.read(reinterpret_cast<char*>(rlePair), sizeof(rlePair));
        if (file.gcount() != sizeof(rlePair)) break;

        uint16_t blockType = rlePair[0];
        uint16_t runLength = rlePair[1];

        if (blockIndex + runLength > BLOCKS_PER_CHUNK) {
            throw std::runtime_error(
                "RLE run length of " + std::to_string(runLength) + " exceeds chunk size at index " +
                std::to_string(blockIndex)
            );
        }

        Block fillBlock = {blockType, blockType != AIR};
        for (uint16_t i = 0; i < runLength; i++) {
            blocks[blockIndex++] = fillBlock;
        }
    }

    if (blockIndex != BLOCKS_PER_CHUNK) {
        throw std::runtime_error(
            "Chunk RLE decoding ended prematurely. Expected " + std::to_string(BLOCKS_PER_CHUNK) + " blocks, but got " +
            std::to_string(blockIndex)
        );
    }

    return std::move(blocks);
}

void ChunkStorage::saveChunkData(const glm::ivec3& chunkPos, const Block* blocks) {
    if (!blocks) return;

    std::shared_ptr<std::mutex> fileAccessMutex = getChunkMutex(chunkPos);
    std::lock_guard<std::mutex> lock(*fileAccessMutex);

    std::string chunkFileName = chunkFileNameFromCoords(chunkPos);
    std::filesystem::path chunkFilePath = m_chunkStoragePath / chunkFileName;

    std::ofstream file(chunkFilePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open chunk file for writing: " + chunkFilePath.string());
    }

    file.write("CHNK", 4);  // Magic number
    uint8_t version = 1;    // Version
    uint8_t flags = 0;      // Reserved for now
    file.write(reinterpret_cast<char*>(&version), 1);
    file.write(reinterpret_cast<char*>(&flags), 1);

    uint16_t lastType = blocks[0].type;
    uint16_t runLength = 1;

    for (size_t i = 1; i < BLOCKS_PER_CHUNK; i++) {
        uint16_t currType = blocks[i].type;

        if (currType == lastType && runLength < UINT16_MAX) {
            runLength++;
        } else {
            // Write type and run length
            file.write(reinterpret_cast<char*>(&lastType), sizeof(uint16_t));
            file.write(reinterpret_cast<char*>(&runLength), sizeof(uint16_t));

            lastType = currType;
            runLength = 1;
        }
    }

    // Write final entry
    file.write(reinterpret_cast<char*>(&lastType), sizeof(uint16_t));
    file.write(reinterpret_cast<char*>(&runLength), sizeof(uint16_t));
}