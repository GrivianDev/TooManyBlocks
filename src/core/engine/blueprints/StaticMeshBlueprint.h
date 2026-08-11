#ifndef TOOMANYBLOCKS_STATICMESHBLUEPRINT_H
#define TOOMANYBLOCKS_STATICMESHBLUEPRINT_H

#include "engine/rendering/RenderData.h"
#include "engine/rendering/StaticMesh.h"
#include "engine/rendering/Vertices.h"
#include "engine/assets/cpu/CPURenderData.h"

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<Vertex>& cpuStaticMesh);

#endif
