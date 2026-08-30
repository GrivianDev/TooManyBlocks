#ifndef TOOMANYBLOCKS_SKELETALMESHCREATE_H
#define TOOMANYBLOCKS_SKELETALMESHCREATE_H

#include "engine/scene/renderables/SkeletalMesh.h"
#include "engine/assets/cpu/CPUSkeletalMeshData.h"

SkeletalMesh::Asset createSkeletalMeshAsset(const CPUSkeletalMeshData& cpuSkeletalMesh);

#endif
