#include "Chunk.h"

#include <stdexcept>

#include "Logger.h"

glm::ivec3 Chunk::worldToChunkOrigin(const glm::vec3& worldPos) {
    glm::vec3 chunkSize(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH);
    glm::ivec3 chunkCoord = glm::floor(glm::floor(worldPos) / chunkSize);
    return chunkCoord * glm::ivec3(chunkSize);
}

glm::ivec3 Chunk::worldToChunkLocal(const glm::ivec3& chunkOrigin, const glm::ivec3& worldBlockPos) {
    return worldBlockPos - chunkOrigin;
}

void Chunk::tryCommitRebuild() {
    if (m_pendingRebuildMesh.isReady()) {
        lgr::lout.debug("Commiting rebuild");
        m_mesh->getAssetHandle() = std::move(m_pendingRebuildMesh);

        m_pendingRebuildMesh.reset();
    }
}

bool isBlockFaceVisible(const Block* blocks, int x, int y, int z, AxisDirection faceDirection) {
    // Check chunk/world bounds
    if (blocks == nullptr) {
        throw std::runtime_error("Block array was null while testing for block visibility");
    }
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) {
        return true;  // Block is on the edge and visible from this direction
    }

    // Access neighboring block based on face direction
    switch (faceDirection) {
        case AxisDirection::NegativeX:  // (x - 1, y, z) LEFT
            return (x - 1 < 0 || !blocks[chunkBlockIndex(x - 1, y, z)].isSolid);
        case AxisDirection::PositiveX:  // (x + 1, y, z) RIGHT
            return (x + 1 >= CHUNK_WIDTH || !blocks[chunkBlockIndex(x + 1, y, z)].isSolid);
        case AxisDirection::PositiveY:  // (x, y + 1, z) TOP
            return (y + 1 >= CHUNK_HEIGHT || !blocks[chunkBlockIndex(x, y + 1, z)].isSolid);
        case AxisDirection::NegativeY:  // (x, y - 1, z) BOTTOM
            return (y - 1 < 0 || !blocks[chunkBlockIndex(x, y - 1, z)].isSolid);
        case AxisDirection::PositiveZ:  // (x, y, z + 1) FRONT
            return (z + 1 >= CHUNK_DEPTH || !blocks[chunkBlockIndex(x, y, z + 1)].isSolid);
        case AxisDirection::NegativeZ:  // (x, y, z - 1) BACK
            return (z - 1 < 0 || !blocks[chunkBlockIndex(x, y, z - 1)].isSolid);
        default: return false;
    }
}