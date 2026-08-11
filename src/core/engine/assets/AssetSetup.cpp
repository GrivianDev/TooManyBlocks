#include "AssetSetup.h"

#include <string>
#include <vector>

#include "AppConstants.h"
#include "engine/assets/EngineAssets.h"
#include "engine/assets/builders/ShaderBuilder.h"
#include "engine/assets/builders/SkeletalMeshBuilder.h"
#include "engine/assets/builders/StaticMeshBuilder.h"
#include "engine/assets/builders/TextureBuilder.h"
#include "engine/assets/loaders/ShaderLoader.h"
#include "engine/assets/loaders/SkeletalMeshLoader.h"
#include "engine/assets/loaders/StaticMeshLoader.h"
#include "engine/assets/loaders/TextureLoader.h"
#include "engine/rendering/lowlevelapi/Shader.h"
#include "engine/rendering/lowlevelapi/Texture.h"
#include "engine/rendering/lowlevelapi/TransformFeedbackShader.h"

struct FileSource {
    std::string path;
};

struct ShaderSource {
    std::string path;
    std::vector<std::string> varyings;
    ShaderDefines defines;
    ShaderLoadOption option;
};

void registerAssetFactories(AssetManager& assets) {
    assets.setDefaultCachePolicy(CachePolicy::RefCounted);

    assets.registerFactory<CPUTexture>([](AssetHandle h, AssetManager& assets) {
        const FileSource& source = assets.getSource<FileSource>(h);
        return Future<CPUTexture>([source]() { return loadTextureFromFile(source.path); }).start();
    });
    assets.setCachePolicy<Texture>(CachePolicy::GracePeriod, 3.0f);
    assets.registerFactory<Texture>([](AssetHandle h, AssetManager& assets) {
        return build(assets.request<CPUTexture>(h));
    });

    assets.registerFactory<CPUShader>([](AssetHandle h, AssetManager& assets) {
        const ShaderSource& source = assets.getSource<ShaderSource>(h);
        return Future<CPUShader>([source]() { return loadShaderFromFile(source.path, source.option); }).start();
    });
    assets.setCachePolicy<Shader>(CachePolicy::GracePeriod, 3.0f);
    assets.registerFactory<Shader>([](AssetHandle h, AssetManager& assets) {
        const ShaderSource& source = assets.getSource<ShaderSource>(h);
        return build(assets.request<CPUShader>(h), source.defines);
    });
    assets.setCachePolicy<TransformFeedbackShader>(CachePolicy::GracePeriod, 3.0f);
    assets.registerFactory<TransformFeedbackShader>([](AssetHandle h, AssetManager& assets) {
        const ShaderSource& source = assets.getSource<ShaderSource>(h);
        return buildTFShader(assets.request<CPUShader>(h), source.varyings, source.defines);
    });

    assets.registerFactory<CPURenderData<Vertex>>([](AssetHandle h, AssetManager& assets) {
        const FileSource& source = assets.getSource<FileSource>(h);
        return Future<CPURenderData<Vertex>>(
                   [source]() { return loadStaticMeshFromObjFile(source.path, true); }
        ).start();
    });
    assets.setCachePolicy<StaticMesh::Asset>(CachePolicy::GracePeriod, 3.0f);
    assets.registerFactory<StaticMesh::Asset>([](AssetHandle h, AssetManager& assets) {
        return build(assets.request<CPURenderData<Vertex>>(h));
    });
    assets.registerFactory<CPUSkeletalMeshData>([](AssetHandle h, AssetManager& assets) {
        const FileSource& source = assets.getSource<FileSource>(h);
        return Future<CPUSkeletalMeshData>(
            [source]() { return loadSkeletalMeshFromGlbFile(source.path, true); }
        ).start();
    });
    assets.setCachePolicy<SkeletalMesh::Asset>(CachePolicy::GracePeriod, 3.0f);
    assets.registerFactory<SkeletalMesh::Asset>([](AssetHandle h, AssetManager& assets) {
        return build(assets.request<CPUSkeletalMeshData>(h));
    });

}

void setupAssets(AssetManager& assets) {
    Assets::Shader::CHUNK_DEPTH = assets.import<ShaderSource>(
        {Res::Shader::CHUNK_DEPTH, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::CHUNK_SSAO_GBUFFER = assets.import<ShaderSource>(
        {Res::Shader::CHUNK_SSAO_GBUFFER, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::CHUNK = assets.import<ShaderSource>(
        {Res::Shader::CHUNK, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::SIMPLE = assets.import<ShaderSource>(
        {Res::Shader::SIMPLE, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::DEPTH = assets.import<ShaderSource>(
        {Res::Shader::DEPTH, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::LINE = assets.import<ShaderSource>(
        {Res::Shader::LINE, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::SSAO_PASS = assets.import<ShaderSource>(
        {Res::Shader::SSAO_PASS, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::SSAO_BLUR = assets.import<ShaderSource>(
        {Res::Shader::SSAO_BLUR, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::SKELETAL_MESH = assets.import<ShaderSource>(
        {Res::Shader::SKELETAL_MESH, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::TRANSPARENT = assets.import<ShaderSource>(
        {Res::Shader::TRANSPARENT, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::RESOLVER = assets.import<ShaderSource>(
        {Res::Shader::RESOLVER, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::FXAA = assets.import<ShaderSource>(
        {Res::Shader::FXAA, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::PARTICLE = assets.import<ShaderSource>(
        {Res::Shader::PARTICLE, {}, ShaderDefines(), ShaderLoadOption::VertexAndFragment}
    );
    Assets::Shader::PARTICLE_TF = assets.import<ShaderSource>(
        {Res::Shader::PARTICLE_TF,
         {"tf_color", "tf_velocity", "tf_position", "tf_timeToLive", "tf_initialTimeToLive", "tf_size", "tf_metadata"},
         ShaderDefines(),
         ShaderLoadOption::VertexOnly}
    );

    Assets::Texture::BLOCK_TEX_ATLAS = assets.import<FileSource>({Res::Texture::BLOCK_TEX_ATLAS});
    Assets::Texture::TESTBLOCK_TEXTURE = assets.import<FileSource>({Res::Texture::TESTBLOCK_TEXTURE});
    Assets::Texture::TESTFLY_TEXTURE = assets.import<FileSource>({Res::Texture::TESTFLY_TEXTURE});

    Assets::Model::TEST_UNIT_BLOCK = assets.import<FileSource>({Res::Model::TEST_UNIT_BLOCK});
    Assets::Model::TESTFLY = assets.import<FileSource>({Res::Model::TESTFLY});
}