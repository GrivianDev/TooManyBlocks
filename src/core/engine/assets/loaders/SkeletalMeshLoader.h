#ifndef TOOMANYBLOCKS_SKELETALMESHLOADER_H
#define TOOMANYBLOCKS_SKELETALMESHLOADER_H

#include <memory>
#include <string>

#include "engine/rendering/Vertices.h"
#include "engine/assets/cpu/CPUSkeletalMeshData.h"

CPUSkeletalMeshData loadSkeletalMeshFromGlbFile(const std::string& glbFilePath, bool flipWinding = false);

#endif
