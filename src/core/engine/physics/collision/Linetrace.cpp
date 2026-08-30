#include "Linetrace.h"

#include <cfloat>

#include "Application.h"
#include "Logger.h"
#include "game/GameInstance.h"
#include "game/world/Chunk.h"
#include "game/world/World.h"

static HitResult optimizedBlockLinetrace(const glm::vec3& start, const glm::vec3& end) {
    ApplicationContext* context = Application::getContext();

    World* world = context->instance->m_world;
    glm::vec3 directionVec = end - start;
    float maxDistance = glm::length(directionVec);
    directionVec = glm::normalize(directionVec);

    glm::ivec3 pos = glm::ivec3(glm::floor(start));
    glm::ivec3 step;
    step.x = directionVec.x > 0 ? 1 : -1;
    step.y = directionVec.y > 0 ? 1 : -1;
    step.z = directionVec.z > 0 ? 1 : -1;

    glm::vec3 deltaDist;
    deltaDist.x = directionVec.x != 0 ? glm::abs(1.0f / directionVec.x) : FLT_MAX;
    deltaDist.y = directionVec.y != 0 ? glm::abs(1.0f / directionVec.y) : FLT_MAX;
    deltaDist.z = directionVec.z != 0 ? glm::abs(1.0f / directionVec.z) : FLT_MAX;

    glm::vec3 tMax;
    tMax.x = directionVec.x > 0 ? (glm::ceil(start.x) - start.x) * deltaDist.x
                                : (start.x - glm::floor(start.x)) * deltaDist.x;
    tMax.y = directionVec.y > 0 ? (glm::ceil(start.y) - start.y) * deltaDist.y
                                : (start.y - glm::floor(start.y)) * deltaDist.y;
    tMax.z = directionVec.z > 0 ? (glm::ceil(start.z) - start.z) * deltaDist.z
                                : (start.z - glm::floor(start.z)) * deltaDist.z;

    float totalDistance = 0.0f;

    bool hit = false;
    glm::vec3 impactPoint(0.0f);

    while (totalDistance < maxDistance) {
        glm::ivec3 currentChunkPos = Chunk::worldToChunkOrigin(pos);
        glm::ivec3 chunkRelPos = Chunk::worldToChunkLocal(currentChunkPos, pos);
        if (Chunk* chunk = world->getChunk(currentChunkPos)) {
            const Block& block = chunk->blocks()[chunkBlockIndex(chunkRelPos.x, chunkRelPos.y, chunkRelPos.z)];
            if (block.isSolid) {
                hit = true;
                impactPoint = start + (totalDistance * directionVec);
                break;
            }
            int minDim = 0;
            if (tMax[1] < tMax[minDim]) minDim = 1;
            if (tMax[2] < tMax[minDim]) minDim = 2;

            totalDistance = tMax[minDim];
            pos[minDim] += step[minDim];
            tMax[minDim] += deltaDist[minDim];
        } else {
            break;
        }
    }

    return {hit, glm::vec3(pos), impactPoint};
}

HitResult linetraceByChannel(const glm::vec3& start, const glm::vec3& end, Channel channel) {
    return optimizedBlockLinetrace(start, end);
}
