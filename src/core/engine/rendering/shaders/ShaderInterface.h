#ifndef TOOMANYBLOCKS_SHADERCONTRACT_H
#define TOOMANYBLOCKS_SHADERCONTRACT_H

#include <string>
#include <unordered_set>
#include <vector>

#include "engine/rendering/shaders/generator/Glsl.h"

struct LocatedValue {
    GlslType glslType;
    unsigned int location;
};

struct ShaderInterface {
    std::vector<LocatedValue> vertexInputs;
    std::vector<LocatedValue> vertexOutputs;
    std::vector<LocatedValue> fragmentOutputs;
    std::vector<std::string> varyingNames;
    std::unordered_set<std::string> uniforms;
};

#endif
