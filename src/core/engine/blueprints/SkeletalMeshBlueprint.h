#ifndef TOOMANYBLOCKS_SKELETALMESHBLUEPRINT_H
#define TOOMANYBLOCKS_SKELETALMESHBLUEPRINT_H

#include "engine/rendering/SkeletalMesh.h"
#include "engine/assets/cpu/CPUSkeletalMeshData.h"

SkeletalMesh::Asset createSkeletalMeshAsset(const CPUSkeletalMeshData& cpuSkeletalMesh);

#endif
