#ifndef TOOMANYBLOCKS_STATICMESHBUILDER_H
#define TOOMANYBLOCKS_STATICMESHBUILDER_H

#include "engine/assets/cpu/CPURenderData.h"
#include "engine/rendering/Vertices.h"
#include "engine/scene/renderables/StaticMesh.h"
#include "foundation/threading/Future.h"

Future<StaticMesh::Asset> build(const Future<CPURenderData<Vertex>>& cpuMesh);

Future<StaticMesh::Asset> build(const Future<CPURenderData<CompactChunkVertex>>& cpuMesh);

#endif
