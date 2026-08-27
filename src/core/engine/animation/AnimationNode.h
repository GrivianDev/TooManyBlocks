#ifndef TOOMANYBLOCKS_ANIMATIONNODE_H
#define TOOMANYBLOCKS_ANIMATIONNODE_H

#include <vector>

#include "datatypes/Transform.h"
#include "engine/Updatable.h"

class AnimationNode : public Updatable {
public:
    virtual ~AnimationNode() = default;

    virtual void evaluate(std::vector<Transform>& outPose) = 0;
};

#endif
