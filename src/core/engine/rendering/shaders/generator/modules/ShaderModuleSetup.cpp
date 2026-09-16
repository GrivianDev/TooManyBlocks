#include "ShaderModuleSetup.h"

#include "engine/assets/AssetPaths.h"

void setupModules(ShaderModuleRegistry& modules) {
    modules.registerModule(ShaderModule(
        Res::Shader::Module::CHUNKS,
        {"decodePosition",
         "decodeTexIndex",
         "decodeUV",
         "decodeNormal",
         "getChunkWorldVertexPosition",
         "getProjectedVertexPosition"}
    ));
    modules.registerModule(
        ShaderModule(Res::Shader::Module::TRANSFORM, {"transformPosition", "transformPosition3D", "transformNormal"})
    );
    modules.registerModule(ShaderModule(Res::Shader::Module::SKINNING, {"calculateSkinMatrix"}, {"JointMatrices"}));
    modules.registerModule(
        ShaderModule(Res::Shader::Module::TEXTURE_ATLAS, {"sampleTextureAtlas", "sampleTextureAtlasFlippedY"})
    );
    modules.registerModule(ShaderModule(
        Res::Shader::Module::PARTICLES_TF,
        {"processParticles"},
        {"ParticleModulesBlock",
         "u_moduleCount",
         "u_spawnCount",
         "u_particleSpawnOffset",
         "u_allocatedParticleCount",
         "u_flags",
         "u_deltaTime",
         "u_time"}
    ));
    modules.registerModule(
        ShaderModule(Res::Shader::Module::PARTICLES, {"calculateVertexOffset", "getParticleTexIndex"})
    );
    modules.registerModule(ShaderModule(Res::Shader::Module::PARTICLES_FRAG, {"discardIfDead"}));
    modules.registerModule(ShaderModule(Res::Shader::Module::ALPHA_TEST_FRAG, {"alphaTest"}));
    modules.registerModule(ShaderModule(Res::Shader::Module::DISTANCE_FADE, {"calculateDistanceFade"}));
    modules.registerModule(ShaderModule(Res::Shader::Module::SSAO_FRAG, {"sampleSSAO"}, {"u_ssaoTexture"}));
    modules.registerModule(ShaderModule(
        Res::Shader::Module::LIGHTS,
        {"accumulateLightContributions"},
        {"LightsBlock", "ShadowMapsBlock", "u_lightCount", "u_specularFactor", "u_specularExponent", "u_shadowMapAtlas"}
    ));
    modules.registerModule(ShaderModule(Res::Shader::Module::LIGHTS_UTIL, {"lightWithAmbient"}));
}