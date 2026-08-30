#ifndef TOOMANYBLOCKS_COLLISION_H
#define TOOMANYBLOCKS_COLLISION_H

#include "engine/geometry/BoundingVolume.h"
#include "engine/scene/Axis.h"
#include "game/world/World.h"

bool aabbIntersects(const BoundingBox& a, const BoundingBox& b);

float sweepAndResolveAxis(const BoundingBox& box, glm::vec3 delta, Axis axis, World* world);

glm::vec3 sweepAndResolve(const BoundingBox& box, glm::vec3 delta, World* world);

#endif
