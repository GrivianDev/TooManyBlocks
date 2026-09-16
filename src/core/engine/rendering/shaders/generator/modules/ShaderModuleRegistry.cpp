#include "ShaderModuleRegistry.h"

#include <stdexcept>

void ShaderModuleRegistry::registerModule(ShaderModule module) {
    m_modules.push_back(std::make_unique<ShaderModule>(std::move(module)));

    ShaderModule* registered = m_modules.back().get();

    for (const std::string& function : registered->functions()) {
        auto [it, inserted] = m_functionProviders.emplace(function, registered);

        if (!inserted) {
            throw std::runtime_error("Shader function '" + function + "' is provided by multiple modules");
        }
    }
}

const ShaderModule* ShaderModuleRegistry::findFunctionProvider(const std::string& function) const {
    auto it = m_functionProviders.find(function);
    if (it == m_functionProviders.end()) return nullptr;

    return it->second;
}
