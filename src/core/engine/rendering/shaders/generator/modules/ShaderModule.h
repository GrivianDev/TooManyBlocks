#ifndef TOOMANYBLOCKS_SHADERMODULE_H
#define TOOMANYBLOCKS_SHADERMODULE_H

#include <string>
#include <unordered_set>

class ShaderModule {
private:
    std::string m_modulePath;
    std::unordered_set<std::string> m_functions;
    std::unordered_set<std::string> m_uniforms;

    std::string m_source;

public:
    /**
     * Shader Modules MUST manually define all functions and uniforms that they offer.
     * (Also indirect ones via #include resolve)
     */
    ShaderModule(
        const std::string& modulePath,
        const std::unordered_set<std::string>& functions,
        const std::unordered_set<std::string>& uniforms = {}
    );

    inline const std::string& modulePath() const { return m_modulePath; }
    inline const std::unordered_set<std::string>& functions() const { return m_functions; }
    inline const std::unordered_set<std::string>& uniforms() const { return m_uniforms; }
    inline const std::string& source() const { return m_source; }

    inline bool providesFunction(const std::string& function) const {
        return m_functions.find(function) != m_functions.end();
    }
};

#endif
