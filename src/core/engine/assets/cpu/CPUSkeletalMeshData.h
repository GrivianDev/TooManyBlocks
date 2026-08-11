#ifndef TOOMANYBLOCKS_CPUSKELETALMESHDATA_H
#define TOOMANYBLOCKS_CPUSKELETALMESHDATA_H

#include <memory>
#include <string>
#include <vector>

#include "engine/animation/Animation.h"
#include "engine/rendering/Vertices.h"
#include "engine/assets/cpu/CPURenderData.h"

struct ChannelDeclare {
    int targetNodeIndex;
    AnimationProperty property;
    std::shared_ptr<TimelineBase> timeline;
};

struct AnimationDeclare {
    std::string name;
    std::vector<ChannelDeclare> channels;
};

struct Node {
    std::string name;
    int parentIndex;  // -1 if root
    std::vector<int> childIndices;
    Transform localTransform;
};

struct CPUSkeletalMeshData {
    CPURenderData<SkeletalVertex> meshData;
    std::vector<Node> nodeArray;
    int animatedMeshNodeIndex;
    std::vector<glm::mat4> inverseBindMatrices;  // indexed by joint index
    std::vector<int> jointNodeIndices;           // indexed by joint index (Needed to build joint matrices)
    std::vector<AnimationDeclare> animations;
};

#endif
