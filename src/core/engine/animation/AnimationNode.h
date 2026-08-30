#ifndef TOOMANYBLOCKS_ANIMATIONNODE_H
#define TOOMANYBLOCKS_ANIMATIONNODE_H

#include <vector>

#include "engine/Updatable.h"
#include "engine/scene/Transform.h"

class AnimationNode : public Updatable {
public:
    virtual ~AnimationNode() = default;

    virtual void evaluate(std::vector<Transform>& outPose) = 0;
};

#endif
