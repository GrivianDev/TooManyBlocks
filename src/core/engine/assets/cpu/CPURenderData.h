#ifndef TOOMANYBLOCKS_CPURENDERDATA_H
#define TOOMANYBLOCKS_CPURENDERDATA_H

#include <string>
#include <vector>

#include "engine/geometry/BoundingVolume.h"

template <typename T>
struct CPURenderData {
    std::string name;
    std::vector<T> vertices;
    std::vector<unsigned int> indices;
    BoundingBox bounds;

    inline bool isIndexed() const { return !indices.empty(); }
};

#endif
