#ifndef TOOMANYBLOCKS_CHUNKMESHBLUEPRINT_H
#define TOOMANYBLOCKS_CHUNKMESHBLUEPRINT_H

#include "engine/assets/cpu/CPURenderData.h"
#include "engine/env/Chunk.h"
#include "engine/rendering/BlockToTextureMapping.h"
#include "engine/rendering/RenderData.h"
#include "engine/rendering/Vertices.h"

CPURenderData<CompactChunkVertex> generateMeshForChunk(const Block* blocks, const BlockToTextureMap& texMap);

CPURenderData<CompactChunkVertex> generateMeshForChunkGreedy(const Block* blocks, const BlockToTextureMap& texMap);

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<CompactChunkVertex>& cpuStaticMesh);

#endif
