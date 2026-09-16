#ifndef TOOMANYBLOCKS_SHADERKEY_H
#define TOOMANYBLOCKS_SHADERKEY_H

#include "engine/rendering/material/Material.h"
#include "engine/rendering/shaders/PassTypes.h"
#include "engine/scene/renderables/GeometryType.h"

struct ShaderFeatures {
    bool lit = false;
    bool baseColorTexture = false;
    bool alphaTest = false;
    bool ssao = false;

    bool operator==(const ShaderFeatures& other) const {
        return lit == other.lit && baseColorTexture == other.baseColorTexture && alphaTest == other.alphaTest &&
               ssao == other.ssao;
    }
};

struct ShaderKey {
    PassType pass;
    GeometryType geometry;

    ShaderFeatures features;

    bool operator==(const ShaderKey& other) const {
        return pass == other.pass && geometry == other.geometry && features == other.features;
    }
};

struct ShaderKeyHash {
    std::size_t operator()(const ShaderKey& key) const {
        std::size_t hash = 0;

        auto combine = [&hash](std::size_t value) { hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2); };

        combine(std::hash<int>{}(static_cast<int>(key.pass)));
        combine(std::hash<int>{}(static_cast<int>(key.geometry)));

        combine(std::hash<bool>{}(key.features.lit));
        combine(std::hash<bool>{}(key.features.baseColorTexture));
        combine(std::hash<bool>{}(key.features.alphaTest));
        combine(std::hash<bool>{}(key.features.ssao));

        return hash;
    }
};

#endif
