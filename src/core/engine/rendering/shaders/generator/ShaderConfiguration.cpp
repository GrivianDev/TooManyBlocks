#include "ShaderConfiguration.h"

#include <stdexcept>

static void configureMeshShader(const ShaderKey& key, ShaderGenerator& shader) {
    auto vertexPosition = shader.vertexInput(0, GlslType::Vec3);
    auto vertexUV = shader.vertexInput(1, GlslType::Vec2);
    auto vertexNormal = shader.vertexInput(2, GlslType::Vec3);

    auto u_mvp = shader.uniform(GlslType::Mat4, "u_mvp");
    auto u_model = shader.uniform(GlslType::Mat4, "u_model");
    auto u_view = shader.uniform(GlslType::Mat4, "u_view");
    auto u_cameraPosition = shader.uniform(GlslType::Vec3, "u_cameraPosition");

    auto localVertexPos4 = shader.call(GlslType::Vec4, "vec4", {vertexPosition, shader.constant(1.0f)});
    auto worldVertexPos4 = u_model * localVertexPos4;
    auto worldNormal = shader.call(GlslType::Vec3, "transformNormal", {u_model, vertexNormal});

    if (key.pass == PassType::Shadow) {
        auto u_lightViewProjection = shader.uniform(GlslType::Mat4, "u_lightViewProjection");
        shader.vertexPosition().assign(u_lightViewProjection * worldVertexPos4);
    } else {
        shader.vertexPosition().assign(u_mvp * localVertexPos4);
    }

    auto position = shader.varying(worldVertexPos4.xyz());
    auto uv = shader.varying(vertexUV);
    auto normal = shader.varying(worldNormal);

    auto texColor = shader.local(GlslType::Vec4, ValueScope::Fragment);
    if (key.features.baseColorTexture) {
        auto u_texture = shader.uniform(GlslType::Sampler2D, "u_texture");
        texColor.assign(shader.call(GlslType::Vec4, "texture", {u_texture, uv}));
    } else {
        texColor.assign(shader.uniform(GlslType::Vec4, "u_color"));
    }

    if (key.features.alphaTest) {
        shader.call(
            ValueScope::Fragment, "alphaTest", {texColor.a(), shader.uniform(GlslType::Float, "u_alphaCutoff")}
        );
    }

    switch (key.pass) {
        case PassType::Opaque: {
            if (key.features.lit) {
                auto lightContrib = shader.call(
                    GlslType::Vec3, "accumulateLightContributions", {u_view, u_cameraPosition, position, normal}
                );
                auto litColor = shader.call(
                    GlslType::Vec3, "lightWithAmbient", {texColor.rgb(), lightContrib, shader.constant(0.15f)}
                );
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {litColor, shader.constant(1.0f)}));
            }
            if (key.features.ssao) {
                auto u_screenResolution = shader.uniform(GlslType::UVec2, "u_screenResolution");
                auto occlusion = shader.call(GlslType::Float, "sampleSSAO", {u_screenResolution});
                auto occludedColor = texColor.rgb() * occlusion;
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {occludedColor, shader.constant(1.0f)}));
            }
            shader.fragmentOutput(0, GlslType::Vec4).assign(texColor);
            break;
        }

        case PassType::Shadow: break;

        case PassType::AmbientOcclusion: {
            auto viewPosition = (u_view * worldVertexPos4).xyz();
            auto viewNormal = shader.call(GlslType::Vec3, "transformNormal", {u_view, worldNormal});

            shader.fragmentOutput(0, GlslType::Vec3).assign(shader.varying(viewPosition));
            shader.fragmentOutput(1, GlslType::Vec3).assign(shader.varying(viewNormal));
            break;
        }

        case PassType::Transparency: {
            auto weight = texColor.a() * shader.constant(8.0f);
            auto accumRgb = texColor.rgb() * texColor.a() * weight;
            auto accumA = texColor.a() * weight;

            auto accum = shader.call(GlslType::Vec4, "vec4", {accumRgb, accumA});
            auto reveal = texColor.a();

            shader.fragmentOutput(0, GlslType::Vec4).assign(accum);
            shader.fragmentOutput(1, GlslType::Float).assign(reveal);
            break;
        }

        default: throw std::runtime_error("Requested unexpected pass for mesh shader generation");
    }
}

static void configureSkeletalMeshShader(const ShaderKey& key, ShaderGenerator& shader) {
    auto vertexPosition = shader.vertexInput(0, GlslType::Vec3);
    auto vertexUV = shader.vertexInput(1, GlslType::Vec2);
    auto vertexNormal = shader.vertexInput(2, GlslType::Vec3);
    auto jointIndices = shader.vertexInput(3, GlslType::UVec4);
    auto jointWeights = shader.vertexInput(4, GlslType::Vec4);

    auto u_mvp = shader.uniform(GlslType::Mat4, "u_mvp");
    auto u_model = shader.uniform(GlslType::Mat4, "u_model");
    auto u_view = shader.uniform(GlslType::Mat4, "u_view");
    auto u_cameraPosition = shader.uniform(GlslType::Vec3, "u_cameraPosition");

    auto skinMatrix = shader.call(GlslType::Mat4, "calculateSkinMatrix", {jointIndices, jointWeights});
    auto localVertexPos4 = shader.call(GlslType::Vec4, "vec4", {vertexPosition, shader.constant(1.0f)});
    auto skinnedVertexPos4 = skinMatrix * localVertexPos4;

    auto worldVertexPos4 = u_model * skinnedVertexPos4;
    auto worldNormal = shader.call(GlslType::Vec3, "transformNormal", {u_model * skinMatrix, vertexNormal});

    // Rasterization position
    if (key.pass == PassType::Shadow) {
        auto u_lightViewProjection = shader.uniform(GlslType::Mat4, "u_lightViewProjection");
        shader.vertexPosition().assign(u_lightViewProjection * worldVertexPos4);
    } else {
        shader.vertexPosition().assign(u_mvp * skinnedVertexPos4);
    }

    auto position = shader.varying(worldVertexPos4.xyz());
    auto uv = shader.varying(vertexUV);
    auto normal = shader.varying(worldNormal);

    auto texColor = shader.local(GlslType::Vec4, ValueScope::Fragment);
    if (key.features.baseColorTexture) {
        auto u_texture = shader.uniform(GlslType::Sampler2D, "u_texture");
        texColor.assign(shader.call(GlslType::Vec4, "texture", {u_texture, uv}));
    } else {
        texColor.assign(shader.uniform(GlslType::Vec4, "u_color"));
    }

    if (key.features.alphaTest) {
        shader.call(ValueScope::Fragment, "alphaTest", {texColor.a(), shader.constant(0.99f)});
    }

    switch (key.pass) {
        case PassType::Opaque: {
            if (key.features.lit) {
                auto lightContrib = shader.call(
                    GlslType::Vec3, "accumulateLightContributions", {u_view, u_cameraPosition, position, normal}
                );
                auto litColor = shader.call(
                    GlslType::Vec3, "lightWithAmbient", {texColor.rgb(), lightContrib, shader.constant(0.15f)}
                );
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {litColor, shader.constant(1.0f)}));
            }
            if (key.features.ssao) {
                auto u_screenResolution = shader.uniform(GlslType::UVec2, "u_screenResolution");
                auto occlusion = shader.call(GlslType::Float, "sampleSSAO", {u_screenResolution});
                auto occludedColor = texColor.rgb() * occlusion;
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {occludedColor, shader.constant(1.0f)}));
            }
            shader.fragmentOutput(0, GlslType::Vec4).assign(texColor);
            break;
        }

        case PassType::Shadow: break;

        case PassType::AmbientOcclusion: {
            auto viewPosition = (u_view * worldVertexPos4).xyz();
            auto viewNormal = shader.call(GlslType::Vec3, "transformNormal", {u_view, worldNormal});

            shader.fragmentOutput(0, GlslType::Vec3).assign(shader.varying(viewPosition));
            shader.fragmentOutput(1, GlslType::Vec3).assign(shader.varying(viewNormal));
            break;
        }

        case PassType::Transparency: {
            auto weight = texColor.a() * shader.constant(8.0f);
            auto accumRgb = texColor.rgb() * texColor.a() * weight;
            auto accumA = texColor.a() * weight;

            auto accum = shader.call(GlslType::Vec4, "vec4", {accumRgb, accumA});
            auto reveal = texColor.a();

            shader.fragmentOutput(0, GlslType::Vec4).assign(accum);
            shader.fragmentOutput(1, GlslType::Float).assign(reveal);
            break;
        }

        default: throw std::runtime_error("Requested unexpected pass for skeletal mesh shader generation");
    }
}

static void configureLineShader(const ShaderKey& key, ShaderGenerator& shader) {
    if (key.pass != PassType::Opaque && key.pass != PassType::Transparency) {
        throw std::runtime_error("Line shaders only support the opaque / transparency pass");
    }
    if (key.features.lit) {
        throw std::runtime_error("Line shaders do not support lighting");
    }

    auto vertexPosition = shader.vertexInput(0, GlslType::Vec3);

    auto u_mvp = shader.uniform(GlslType::Mat4, "u_mvp");
    auto texColor = shader.local(GlslType::Vec4, ValueScope::Fragment);
    texColor.assign(shader.uniform(GlslType::Vec4, "u_color"));

    auto vertexPosition4 = shader.call(GlslType::Vec4, "vec4", {vertexPosition, shader.constant(1.0f)});

    shader.vertexPosition().assign(u_mvp * vertexPosition4);

    if (key.features.alphaTest) {
        shader.call(
            ValueScope::Fragment, "alphaTest", {texColor.a(), shader.uniform(GlslType::Float, "u_alphaCutoff")}
        );
    }

    switch (key.pass) {
        case PassType::Opaque: {
            shader.fragmentOutput(0, GlslType::Vec4).assign(texColor);
            break;
        }

        case PassType::Transparency: {
            auto weight = texColor.a() * shader.constant(8.0f);
            auto accumRgb = texColor.rgb() * texColor.a() * weight;
            auto accumA = texColor.a() * weight;

            auto accum = shader.call(GlslType::Vec4, "vec4", {accumRgb, accumA});
            auto reveal = texColor.a();

            shader.fragmentOutput(0, GlslType::Vec4).assign(accum);
            shader.fragmentOutput(1, GlslType::Float).assign(reveal);
            break;
        }

        default: throw std::runtime_error("Requested unexpected pass for chunk shader generation");
    }
}

static void configureParticleTFShader(const ShaderKey& key, ShaderGenerator& shader) {
    auto color = shader.local(GlslType::Vec4, ValueScope::Vertex);
    auto velocity = shader.local(GlslType::Vec3, ValueScope::Vertex);
    auto position = shader.local(GlslType::Vec3, ValueScope::Vertex);
    auto timeToLive = shader.local(GlslType::Float, ValueScope::Vertex);
    auto initialTimeToLive = shader.local(GlslType::Float, ValueScope::Vertex);
    auto size = shader.local(GlslType::Float, ValueScope::Vertex);
    auto metadata = shader.local(GlslType::UInt, ValueScope::Vertex);

    color.assign(shader.vertexInput(0, GlslType::Vec4));
    velocity.assign(shader.vertexInput(1, GlslType::Vec3));
    position.assign(shader.vertexInput(2, GlslType::Vec3));
    timeToLive.assign(shader.vertexInput(3, GlslType::Float));
    initialTimeToLive.assign(shader.vertexInput(4, GlslType::Float));
    size.assign(shader.vertexInput(5, GlslType::Float));
    metadata.assign(shader.vertexInput(6, GlslType::UInt));

    shader.call(
        ValueScope::Vertex,
        "processParticles",
        {color, velocity, position, timeToLive, initialTimeToLive, size, metadata}
    );

    shader.vertexOutput(0, GlslType::Vec4).assign(color);
    shader.vertexOutput(1, GlslType::Vec3).assign(velocity);
    shader.vertexOutput(2, GlslType::Vec3).assign(position);
    shader.vertexOutput(3, GlslType::Float).assign(timeToLive);
    shader.vertexOutput(4, GlslType::Float).assign(initialTimeToLive);
    shader.vertexOutput(5, GlslType::Float).assign(size);
    shader.vertexOutput(6, GlslType::UInt).assign(metadata);
}

static void configureParticleShader(const ShaderKey& key, ShaderGenerator& shader) {
    if (key.pass == PassType::TransformFeedback) {
        configureParticleTFShader(key, shader);
        return;
    }

    auto vertexPosition = shader.vertexInput(0, GlslType::Vec2);
    auto vertexUv = shader.vertexInput(1, GlslType::Vec2);

    auto in_color = shader.vertexInput(2, GlslType::Vec4);
    auto in_velocity = shader.vertexInput(3, GlslType::Vec3);
    auto in_position = shader.vertexInput(4, GlslType::Vec3);
    auto in_timeToLive = shader.vertexInput(5, GlslType::Float);
    auto in_initialTimeToLive = shader.vertexInput(6, GlslType::Float);
    auto in_size = shader.vertexInput(7, GlslType::Float);
    auto in_metadata = shader.vertexInput(8, GlslType::UInt);

    auto u_mvp = shader.uniform(GlslType::Mat4, "u_mvp");
    auto u_cameraRight = shader.uniform(GlslType::Vec3, "u_cameraRight");
    auto u_cameraUp = shader.uniform(GlslType::Vec3, "u_cameraUp");

    auto decodedTexIndex = shader.call(GlslType::UInt, "getParticleTexIndex", {in_metadata});

    auto offset = shader.call(
        GlslType::Vec3, "calculateVertexOffset", {vertexPosition, in_size, u_cameraRight, u_cameraUp}
    );
    auto position4 = shader.call(GlslType::Vec4, "vec4", {in_position + offset, shader.constant(1.0f)});
    shader.vertexPosition().assign(u_mvp * position4);

    auto uv = shader.varying(vertexUv);
    auto color = shader.varying(in_color, VaryingInterpolation::Flat);
    auto texIndex = shader.varying(decodedTexIndex, VaryingInterpolation::Flat);
    auto timeToLive = shader.varying(in_timeToLive, VaryingInterpolation::Flat);

    auto u_textureAtlas = shader.uniform(GlslType::Sampler2D, "u_textureAtlas");
    auto u_textureSize = shader.uniform(GlslType::UInt, "u_textureSize");

    shader.call(ValueScope::Fragment, "discardIfDead", {timeToLive});

    auto texColor = shader.local(GlslType::Vec4, ValueScope::Fragment);
    if (key.features.baseColorTexture) {
        texColor.assign(
            shader.call(GlslType::Vec4, "sampleTextureAtlasFlippedY", {u_textureAtlas, uv, texIndex, u_textureSize})
                .rgba()
        );
        texColor.assign(texColor * color);
    } else {
        texColor.assign(color);
    }

    if (key.features.alphaTest) {
        shader.call(
            ValueScope::Fragment, "alphaTest", {texColor.a(), shader.uniform(GlslType::Float, "u_alphaCutoff")}
        );
    }

    switch (key.pass) {
        case PassType::Opaque: {
            shader.fragmentOutput(0, GlslType::Vec4).assign(texColor);
            break;
        }

        case PassType::Transparency: {
            auto weight = texColor.a() * shader.constant(8.0f);
            auto accumRgb = texColor.rgb() * texColor.a() * weight;
            auto accumA = texColor.a() * weight;

            auto accum = shader.call(GlslType::Vec4, "vec4", {accumRgb, accumA});
            auto reveal = texColor.a();

            shader.fragmentOutput(0, GlslType::Vec4).assign(accum);
            shader.fragmentOutput(1, GlslType::Float).assign(reveal);
            break;
        }

        default: throw std::runtime_error("Requested unexpected pass for chunk shader generation");
    }
}

static void configureChunkShader(const ShaderKey& key, ShaderGenerator& shader) {
    auto compressedPosition = shader.vertexInput(0, GlslType::UInt);
    auto compressedData = shader.vertexInput(1, GlslType::UInt);

    auto u_view = shader.uniform(GlslType::Mat4, "u_view");
    auto u_cameraPosition = shader.uniform(GlslType::Vec3, "u_cameraPosition");
    auto u_position = shader.uniform(GlslType::Vec3, "u_position");

    auto localPosInChunk = shader.call(GlslType::Vec3, "decodePosition", {compressedPosition});
    auto decodedTexIndex = shader.call(GlslType::UInt, "decodeTexIndex", {compressedData});
    auto decodedUV = shader.call(GlslType::Vec2, "decodeUV", {compressedData});
    auto decodedNormal = shader.call(GlslType::Vec3, "decodeNormal", {compressedData});

    auto worldVertexPos = u_position + localPosInChunk;
    auto worldVertexPos4 = shader.call(GlslType::Vec4, "vec4", {worldVertexPos, shader.constant(1.0f)});

    if (key.pass == PassType::Shadow) {
        auto u_lightViewProjection = shader.uniform(GlslType::Mat4, "u_lightViewProjection");
        shader.vertexPosition().assign(u_lightViewProjection * worldVertexPos4);
    } else {
        auto u_viewProjection = shader.uniform(GlslType::Mat4, "u_viewProjection");
        shader.vertexPosition().assign(u_viewProjection * worldVertexPos4);
    }

    auto position = shader.varying(worldVertexPos);
    auto texIndex = shader.varying(decodedTexIndex, VaryingInterpolation::Flat);
    auto uv = shader.varying(decodedUV);
    auto normal = shader.varying(decodedNormal, VaryingInterpolation::Flat);

    auto u_textureAtlas = shader.uniform(GlslType::Sampler2D, "u_textureAtlas");
    auto u_textureSize = shader.uniform(GlslType::UInt, "u_textureSize");

    auto texColor = shader.local(GlslType::Vec4, ValueScope::Fragment);
    if (key.features.baseColorTexture) {
        auto uvFrag = shader.call(GlslType::Vec2, "fract", {uv});
        texColor.assign(
            shader.call(GlslType::Vec4, "sampleTextureAtlas", {u_textureAtlas, uvFrag, texIndex, u_textureSize})
        );
    } else {
        texColor.assign(shader.uniform(GlslType::Vec4, "u_color"));
    }

    if (key.features.alphaTest) {
        shader.call(
            ValueScope::Fragment, "alphaTest", {texColor.a(), shader.uniform(GlslType::Float, "u_alphaCutoff")}
        );
    }

    switch (key.pass) {
        case PassType::Opaque: {
            if (key.features.lit) {
                auto lightContrib = shader.call(
                    GlslType::Vec3, "accumulateLightContributions", {u_view, u_cameraPosition, position, normal}
                );
                auto litColor = shader.call(
                    GlslType::Vec3, "lightWithAmbient", {texColor.rgb(), lightContrib, shader.constant(0.15f)}
                );
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {litColor, shader.constant(1.0f)}));
            }
            if (key.features.ssao) {
                auto u_screenResolution = shader.uniform(GlslType::UVec2, "u_screenResolution");
                auto occlusion = shader.call(GlslType::Float, "sampleSSAO", {u_screenResolution});
                auto occludedColor = texColor.rgb() * occlusion;
                texColor.assign(shader.call(GlslType::Vec4, "vec4", {occludedColor, shader.constant(1.0f)}));
            }
            shader.fragmentOutput(0, GlslType::Vec4).assign(texColor);
            break;
        }

        case PassType::Shadow: break;

        case PassType::AmbientOcclusion: {
            auto viewPosition = (u_view * worldVertexPos4).xyz();
            auto viewNormal = shader.call(GlslType::Vec3, "transformNormal", {u_view, decodedNormal});

            shader.fragmentOutput(0, GlslType::Vec3).assign(shader.varying(viewPosition));
            shader.fragmentOutput(1, GlslType::Vec3).assign(shader.varying(viewNormal, VaryingInterpolation::Flat));
            break;
        }

        case PassType::Transparency: {
            auto weight = texColor.a() * shader.constant(8.0f);
            auto accumRgb = texColor.rgb() * texColor.a() * weight;
            auto accumA = texColor.a() * weight;

            auto accum = shader.call(GlslType::Vec4, "vec4", {accumRgb, accumA});
            auto reveal = texColor.a();

            shader.fragmentOutput(0, GlslType::Vec4).assign(accum);
            shader.fragmentOutput(1, GlslType::Float).assign(reveal);
            break;
        }

        default: throw std::runtime_error("Requested unexpected pass for chunk shader generation");
    }
}

void configureShader(const ShaderKey& key, ShaderGenerator& shader) {
    switch (key.geometry) {
        case GeometryType::Mesh: configureMeshShader(key, shader); break;
        case GeometryType::SkeletalMesh: configureSkeletalMeshShader(key, shader); break;
        case GeometryType::Line: configureLineShader(key, shader); break;
        case GeometryType::Particle: configureParticleShader(key, shader); break;
        case GeometryType::Chunk: configureChunkShader(key, shader); break;

        default: throw std::runtime_error("Shader configuration missing for this geometry");
    }
}