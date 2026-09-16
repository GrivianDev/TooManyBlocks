#ifndef TOOMANYBLOCKS_CHUNKMESH_H
#define TOOMANYBLOCKS_CHUNKMESH_H

#include "engine/scene/renderables/StaticMesh.h"

class ChunkMesh : public StaticMesh {
public:
    using StaticMesh::StaticMesh;

    inline GeometryType geometryType() const override { return GeometryType::Chunk; };
};

#endif
