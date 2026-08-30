#ifndef TOOMANYBLOCKS_CHUNKMESHGENERATION_H
#define TOOMANYBLOCKS_CHUNKMESHGENERATION_H

#include "engine/assets/cpu/CPURenderData.h"
#include "engine/rendering/Vertices.h"
#include "game/blocks/BlockToTextureMapping.h"
#include "game/world/Chunk.h"

CPURenderData<CompactChunkVertex> generateMeshForChunk(const Block* blocks, const BlockToTextureMap& texMap);

CPURenderData<CompactChunkVertex> generateMeshForChunkGreedy(const Block* blocks, const BlockToTextureMap& texMap);

#endif
