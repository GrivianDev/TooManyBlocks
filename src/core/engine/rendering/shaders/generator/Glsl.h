#ifndef TOOMANYBLOCKS_GLSL_H
#define TOOMANYBLOCKS_GLSL_H

#include <stdexcept>

enum class GlslType {
    Float,
    Vec2,
    Vec3,
    Vec4,

    Int,
    IVec2,
    IVec3,
    IVec4,

    UInt,
    UVec2,
    UVec3,
    UVec4,

    Bool,
    BVec2,
    BVec3,
    BVec4,

    Mat2,
    Mat3,
    Mat4,

    Sampler2D,
    Sampler3D,
    SamplerCube,
    Sampler2DArray,

    ISampler2D,
    ISampler3D,
    ISamplerCube,
    ISampler2DArray,

    USampler2D,
    USampler3D,
    USamplerCube,
    USampler2DArray,

    Sampler2DShadow,
    SamplerCubeShadow,
    Sampler2DArrayShadow,
};

enum class ValueScope {
    Vertex,
    Fragment,
};

enum class VaryingInterpolation {
    Smooth,
    Flat
};

struct ShaderAvailability {
    bool vertex = false;
    bool fragment = false;

    inline bool available(ValueScope scope) const { return scope == ValueScope::Vertex ? vertex : fragment; }

    inline bool any() const { return vertex || fragment; }

    inline bool both() const { return vertex && fragment; }

    inline bool singleStage() const { return vertex != fragment; }

    inline ValueScope singleScope() const {
        if (vertex && !fragment) {
            return ValueScope::Vertex;
        } else if (!vertex && fragment) {
            return ValueScope::Fragment;
        }
        throw std::runtime_error("ShaderGenerator: value does not belong to exactly one shader stage");
    }
};

#endif
