#ifndef TOOMANYBLOCKS_ENGINEASSETS_H
#define TOOMANYBLOCKS_ENGINEASSETS_H

#include "engine/assets/AssetManager.h"

namespace Assets {
    namespace Shader {
        inline AssetHandle SSAO_PASS;
        inline AssetHandle SSAO_BLUR;
        inline AssetHandle RESOLVER;
        inline AssetHandle FXAA;
    }  // namespace Shader

    namespace Texture {
        inline AssetHandle BLOCK_TEX_ATLAS;
        inline AssetHandle TESTBLOCK_TEXTURE;
        inline AssetHandle TESTFLY_TEXTURE;
        inline AssetHandle HUMANOID_TEXTURE;
    }  // namespace Texture

    namespace Model {
        inline AssetHandle TEST_UNIT_BLOCK;
        inline AssetHandle TESTFLY;
        inline AssetHandle HUMANOID;
    }  // namespace Model
}  // namespace Assets

#endif
