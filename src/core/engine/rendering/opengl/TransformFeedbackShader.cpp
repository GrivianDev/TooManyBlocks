#include "TransformFeedbackShader.h"

#include <GL/glew.h>

#include "engine/rendering/GLUtils.h"
#include "foundation/util/Utility.h"

TransformFeedbackShader::TransformFeedbackShader(
    const std::unordered_map<unsigned int, std::string>& shaderTypeAndSource,
    const std::vector<std::string>& varyings,
    const ShaderDefines& defines
)
    : Shader(shaderTypeAndSource, defines, true), m_varyings(varyings) {
    // Setup varyings
    std::vector<const char*> cstrings;
    for (const std::string& s : m_varyings) {
        cstrings.push_back(s.c_str());
    }

    GLCALL(glTransformFeedbackVaryings(m_rendererId, cstrings.size(), cstrings.data(), GL_INTERLEAVED_ATTRIBS));

    link();
}

TransformFeedbackShader TransformFeedbackShader::create(
    const std::string& vertexShaderCode,
    const std::vector<std::string>& varyings,
    const ShaderDefines& defines
) {
    return TransformFeedbackShader({{GL_VERTEX_SHADER, vertexShaderCode}}, varyings, defines);
}

TransformFeedbackShader::TransformFeedbackShader(TransformFeedbackShader&& other) noexcept
    : Shader(std::move(other)), m_varyings(std::move(other.m_varyings)) {}

TransformFeedbackShader& TransformFeedbackShader::operator=(TransformFeedbackShader&& other) noexcept {
    if (this != &other) {
        Shader::operator=(std::move(other));

        m_varyings = std::move(other.m_varyings);
    }
    return *this;
}
