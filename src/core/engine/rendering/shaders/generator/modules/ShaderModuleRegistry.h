#ifndef TOOMANYBLOCKS_SHADERMODULEREGISTRY_H
#define TOOMANYBLOCKS_SHADERMODULEREGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/rendering/shaders/generator/modules/ShaderModule.h"

class ShaderModuleRegistry {
private:
    std::vector<std::unique_ptr<ShaderModule>> m_modules;
    std::unordered_map<std::string, ShaderModule*> m_functionProviders;

public:
    void registerModule(ShaderModule module);

    const ShaderModule* findFunctionProvider(const std::string& function) const;
};

#endif
