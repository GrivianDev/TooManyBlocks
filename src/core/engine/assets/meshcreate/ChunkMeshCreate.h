#ifndef TOOMANYBLOCKS_CHUNKMESCREATE_H
#define TOOMANYBLOCKS_CHUNKMESCREATE_H

#include "engine/assets/cpu/CPURenderData.h"
#include "engine/rendering/RenderData.h"
#include "engine/rendering/Vertices.h"
#include "engine/scene/renderables/StaticMesh.h"

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<CompactChunkVertex>& cpuStaticMesh);

#endif
