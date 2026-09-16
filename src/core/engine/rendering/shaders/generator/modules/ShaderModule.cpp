#include "ShaderModule.h"

#include "engine/assets/loaders/ShaderLoader.h"

ShaderModule::ShaderModule(
    const std::string& modulePath,
    const std::unordered_set<std::string>& functions,
    const std::unordered_set<std::string>& uniforms
)
    : m_modulePath(modulePath),
      m_functions(functions),
      m_uniforms(uniforms),
      m_source(loadShaderFromFile(modulePath)) {}