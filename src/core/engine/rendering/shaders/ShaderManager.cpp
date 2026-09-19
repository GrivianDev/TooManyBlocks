#include "ShaderManager.h"

#include "Logger.h"
#include "engine/rendering/opengl/TransformFeedbackShader.h"
#include "engine/rendering/shaders/generator/ShaderConfiguration.h"
#include "engine/rendering/shaders/generator/ShaderGenerator.h"
#include "engine/rendering/shaders/generator/modules/ShaderModuleSetup.h"

RenderProgram& ShaderManager::loadProgram(const ShaderKey& key) {
    auto it = m_programs.find(key);
    if (it != m_programs.end()) return it->second;

    ShaderGenerator generator(&m_modules);
    configureShader(key, generator);
    ShaderContract contract = generator.generate();

    try {
        if (key.pass == PassType::TransformFeedback) {
            auto shader = std::make_unique<TransformFeedbackShader>(
                TransformFeedbackShader::create(contract.cpuShader.vertexShader, contract.interface.varyingNames)
            );
            auto [insertedIt, inserted] = m_programs.emplace(
                key, RenderProgram(std::move(shader), std::move(contract.interface))
            );
            return insertedIt->second;
        } else {
            auto shader = std::make_unique<Shader>(
                Shader::create(contract.cpuShader.vertexShader, contract.cpuShader.fragmentShader)
            );
            auto [insertedIt, inserted] = m_programs.emplace(
                key, RenderProgram(std::move(shader), std::move(contract.interface))
            );
            return insertedIt->second;
        }
    } catch (const std::runtime_error& e) {
        lgr::lout.error("Problematic vertex shader:\n" + contract.cpuShader.vertexShader);
        lgr::lout.error("Problematic fragment shader:\n" + contract.cpuShader.fragmentShader);
        throw e;
    }
}

void ShaderManager::setup() { setupModules(m_modules); }
