#ifndef TOOMANYBLOCKS_SKELETALMESHBUILDER_H
#define TOOMANYBLOCKS_SKELETALMESHBUILDER_H

#include "engine/assets/cpu/CPUSkeletalMeshData.h"
#include "engine/scene/renderables/SkeletalMesh.h"
#include "foundation/threading/Future.h"

Future<SkeletalMesh::Asset> build(const Future<CPUSkeletalMeshData>& cpuMesh);

#endif
