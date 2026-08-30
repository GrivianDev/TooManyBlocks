#ifndef TOOMANYBLOCKS_ASSETPATHS_H
#define TOOMANYBLOCKS_ASSETPATHS_H

namespace Res {
    namespace Shader {
        constexpr const char* CHUNK_DEPTH = "res/shaders/chunkDepthShader";
        constexpr const char* CHUNK_SSAO_GBUFFER = "res/shaders/chunkSSAO_GBufferShader";
        constexpr const char* CHUNK = "res/shaders/chunkShader";
        constexpr const char* SIMPLE = "res/shaders/simpleShader";
        constexpr const char* DEPTH = "res/shaders/depthShader";
        constexpr const char* LINE = "res/shaders/lineShader";
        constexpr const char* SSAO_PASS = "res/shaders/SSAO_PassShader";
        constexpr const char* SSAO_BLUR = "res/shaders/SSAO_BlurShader";
        constexpr const char* SKELETAL_MESH = "res/shaders/skeletalMeshShader";
        constexpr const char* SKELETAL_MESH_DEPTH = "res/shaders/skeletalMeshDepthShader";
        constexpr const char* TRANSPARENT = "res/shaders/transparencyShader";
        constexpr const char* RESOLVER = "res/shaders/resolverShader";
        constexpr const char* FXAA = "res/shaders/fxaaShader";
        constexpr const char* PARTICLE = "res/shaders/particleShader";
        constexpr const char* PARTICLE_TF = "res/shaders/particleTFShader";
    }  // namespace Shader

    namespace Texture {
        constexpr const char* BLOCK_TEX_ATLAS = "res/textures/blockTexAtlas.png";
        constexpr const char* TESTBLOCK_TEXTURE = "res/textures/testTexture.png";
        constexpr const char* TESTFLY_TEXTURE = "res/textures/flyTexture.png";
        constexpr const char* HUMANOID_TEXTURE = "res/textures/humanoidTexture.png";
    }  // namespace Texture

    namespace Font {
        constexpr const char* PROGGY_CLEAN = "res/fonts/ProggyClean.ttf";
        constexpr const char* PROGGY_TINY = "res/fonts/ProggyTiny.ttf";
    }  // namespace Font

    namespace Model {
        constexpr const char* TEST_UNIT_BLOCK = "res/models/testUnitBlock.obj";
        constexpr const char* TESTFLY = "res/models/testFly.glb";
        constexpr const char* HUMANOID = "res/models/Humanoid.glb";
    }  // namespace Model
};  // namespace Res

#endif
