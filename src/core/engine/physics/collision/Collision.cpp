#include "Collision.h"

static constexpr float SWEEP_TOLERANCE = 0.0001f;

bool aabbIntersects(const BoundingBox& a, const BoundingBox& b) {
    return (a.min.x < b.max.x && a.max.x > b.min.x) && (a.min.y < b.max.y && a.max.y > b.min.y) &&
           (a.min.z < b.max.z && a.max.z > b.min.z);
}

float sweepAndResolveAxis(const BoundingBox& box, glm::vec3 delta, Axis axis, World* world) {
    // Only apply movement along the given axis
    glm::vec3 movement(0.0f);
    movement[axis] = delta[axis];

    // Determine which blocks to check for collisions within the swept volume
    BoundingBox movedBox = box.movedBy(movement);
    glm::ivec3 blockStart = glm::floor(glm::min(movedBox.min, box.min));
    glm::ivec3 blockEnd = glm::ivec3(glm::floor(glm::max(movedBox.max, box.max))) + glm::ivec3(1);

    // Inital delta
    float adjustedDelta = delta[axis];

    for (int x = blockStart.x; x < blockEnd.x; x++) {
        for (int y = blockStart.y; y < blockEnd.y; y++) {
            for (int z = blockStart.z; z < blockEnd.z; z++) {
                glm::ivec3 bpos(x, y, z);
                glm::ivec3 chunkPos = Chunk::worldToChunkOrigin(bpos);
                Chunk* chunk = world->getChunk(chunkPos);
                if (!chunk) continue;  // Skip if chunk is not loaded

                glm::ivec3 rel = Chunk::worldToChunkLocal(chunkPos, bpos);
                const Block& block = chunk->blocks()[chunkBlockIndex(rel.x, rel.y, rel.z)];
                if (!block.isSolid) continue;  // Skip non solid blocks

                BoundingBox blockBox = {glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1)};

                // Fix for getting stuck at specific edges / walls (probably a float rounding artifact)
                blockBox.min[axis] -= SWEEP_TOLERANCE;
                blockBox.max[axis] += SWEEP_TOLERANCE;

                if (aabbIntersects(movedBox, blockBox)) {
                    if (adjustedDelta > 0.0f) {
                        // Moving in positive direction — clamp to the near face of the block
                        float newDelta = blockBox.min[axis] - box.max[axis];
                        adjustedDelta = std::min(adjustedDelta, newDelta);
                        if (adjustedDelta < 0.0f) adjustedDelta = 0.0f;
                    } else {
                        // Moving in negative direction — clamp to the far face of the block
                        float newDelta = blockBox.max[axis] - box.min[axis];
                        adjustedDelta = std::max(adjustedDelta, newDelta);
                        if (adjustedDelta > 0.0f) adjustedDelta = 0.0f;
                    }
                }
            }
        }
    }

    // Return the corrected delta, which is guranteed to not overlap smth on this axis
    return adjustedDelta;
}

glm::vec3 sweepAndResolve(const BoundingBox& box, glm::vec3 delta, World* world) {
    BoundingBox current = box;
    delta.x = sweepAndResolveAxis(current, delta, Axis::X, world);
    current = current.movedBy(glm::vec3(delta.x, 0.0f, 0.0f));
    delta.y = sweepAndResolveAxis(current, delta, Axis::Y, world);
    current = current.movedBy(glm::vec3(0.0f, delta.y, 0.0f));
    delta.z = sweepAndResolveAxis(current, delta, Axis::Z, world);
    return delta;
}