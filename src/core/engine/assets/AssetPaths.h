#ifndef TOOMANYBLOCKS_ASSETPATHS_H
#define TOOMANYBLOCKS_ASSETPATHS_H

namespace Res {
    namespace Shader {
        constexpr const char* SSAO_PASS = "res/shaders/SSAO_PassShader";
        constexpr const char* SSAO_BLUR = "res/shaders/SSAO_BlurShader";
        constexpr const char* RESOLVER = "res/shaders/resolverShader";
        constexpr const char* FXAA = "res/shaders/fxaaShader";

        namespace Module {
            constexpr const char* ALPHA_TEST_FRAG = "res/shaders/modules/alphaTest.frag";
            constexpr const char* CHUNKS = "res/shaders/modules/chunks.glsl";
            constexpr const char* DISTANCE_FADE = "res/shaders/modules/distanceFade.glsl";
            constexpr const char* LIGHTS = "res/shaders/modules/lights.glsl";
            constexpr const char* LIGHTS_UTIL = "res/shaders/modules/lightUtil.glsl";
            constexpr const char* PARTICLES_FRAG = "res/shaders/modules/particles.frag";
            constexpr const char* PARTICLES = "res/shaders/modules/particles.glsl";
            constexpr const char* PARTICLES_TF = "res/shaders/modules/particleTF.vert";
            constexpr const char* SKINNING = "res/shaders/modules/skinning.glsl";
            constexpr const char* SSAO_FRAG = "res/shaders/modules/ssao.frag";
            constexpr const char* TEXTURE_ATLAS = "res/shaders/modules/textureAtlas.glsl";
            constexpr const char* TRANSFORM = "res/shaders/modules/transform.glsl";
        };  // namespace Module
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
