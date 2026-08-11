#include "ShaderLoader.h"

#include "foundation/util/Utility.h"

CPUShader loadShaderFromFile(const std::string& shaderPath, ShaderLoadOption option) {
    CPUShader shader;

    size_t pos = shaderPath.find_last_of("/\\");
    std::string basename = shaderPath;
    if (pos != std::string::npos) {
        basename = shaderPath.substr(pos + 1);
    }

    if (option == ShaderLoadOption::VertexAndFragment || option == ShaderLoadOption::VertexOnly) {
        std::string vertFile = shaderPath + "/" + basename + ".vert";
        shader.vertexShader = readFile(vertFile);
    }
    if (option == ShaderLoadOption::VertexAndFragment || option == ShaderLoadOption::FragmentOnly) {
        std::string fragFile = shaderPath + "/" + basename + ".frag";
        shader.fragmentShader = readFile(fragFile);
    }

    return shader;
}