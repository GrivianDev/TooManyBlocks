#ifndef TOOMANYBLOCKS_BLOCKTOTEXTUREMAPPING_H
#define TOOMANYBLOCKS_BLOCKTOTEXTUREMAPPING_H

#include <array>
#include <cstdint>
#include <unordered_map>

#include "engine/scene/Axis.h"
#include "game/blocks/BlockTypes.h"

struct FaceInfo {
    uint16_t texIndex;
    uint8_t animFrames;
};

class BlockToTextureMap {
private:
    std::unordered_map<uint16_t, std::array<FaceInfo, 6>> map;

public:
    BlockToTextureMap();

    FaceInfo getInfo(uint16_t blockType, AxisDirection dir) const;
};

#endif
