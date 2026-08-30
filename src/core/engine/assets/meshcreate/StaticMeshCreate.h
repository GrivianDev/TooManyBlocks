#ifndef TOOMANYBLOCKS_STATICMESHCREATE_H
#define TOOMANYBLOCKS_STATICMESHCREATE_H

#include "engine/rendering/RenderData.h"
#include "engine/scene/renderables/StaticMesh.h"
#include "engine/rendering/Vertices.h"
#include "engine/assets/cpu/CPURenderData.h"

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<Vertex>& cpuStaticMesh);

#endif
