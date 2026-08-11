#include "ShaderBuilder.h"

Future<Shader> build(const Future<CPUShader>& cpuShader, const ShaderDefines& defines) {
    Future<Shader> shaderFuture(
        [cpuShader, defines]() {
            const CPUShader& cpu = cpuShader.value();
            return Shader::create(cpu.vertexShader, cpu.fragmentShader, defines);
        },
        DEFAULT_TASKCONTEXT,
        Executor::Main
    );

    return shaderFuture.start();
}

Future<TransformFeedbackShader> buildTFShader(
    const Future<CPUShader>& cpuShader,
    const std::vector<std::string>& varyings,
    const ShaderDefines& defines
) {
    Future<TransformFeedbackShader> tfFuture(
        [cpuShader, varyings, defines]() {
            const CPUShader& cpu = cpuShader.value();
            return TransformFeedbackShader::create(cpu.vertexShader, varyings, defines);
        },
        DEFAULT_TASKCONTEXT,
        Executor::Main
    );

    return tfFuture.start();
}