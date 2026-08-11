#ifndef TOOMANYBLOCKS_SHADERLOADER_H
#define TOOMANYBLOCKS_SHADERLOADER_H

#include <string>

#include "engine/assets/cpu/CPUShader.h"

enum class ShaderLoadOption {
    VertexAndFragment,
    VertexOnly,
    FragmentOnly
};

CPUShader loadShaderFromFile(const std::string& shaderPath, ShaderLoadOption option);

#endif
