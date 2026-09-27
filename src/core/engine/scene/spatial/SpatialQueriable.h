#ifndef TOOMANYBLOCKS_SPATIALQUERIABLE_H
#define TOOMANYBLOCKS_SPATIALQUERIABLE_H

#include <functional>

#include "engine/geometry/BoundingVolume.h"

template<typename T>
class SpatialQueriable {
public:
    virtual void query(const BoundingBox& bounds, std::function<void(T)> callback) const = 0;
};

#endif
