#ifndef TOOMANYBLOCKS_SHADERBUILDER_H
#define TOOMANYBLOCKS_SHADERBUILDER_H

#include "engine/assets/cpu/CPUShader.h"
#include "engine/rendering/lowlevelapi/Shader.h"
#include "engine/rendering/lowlevelapi/TransformFeedbackShader.h"
#include "foundation/threading/Future.h"

Future<Shader> build(const Future<CPUShader>& cpuShader, const ShaderDefines& defines = ShaderDefines());

Future<TransformFeedbackShader> buildTFShader(
    const Future<CPUShader>& cpuShader,
    const std::vector<std::string>& varyings,
    const ShaderDefines& defines = ShaderDefines()
);

#endif
