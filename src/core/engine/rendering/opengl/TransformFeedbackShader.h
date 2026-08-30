#ifndef TOOMANYBLOCKS_TRANSFORMFEEDBACKSHADER_H
#define TOOMANYBLOCKS_TRANSFORMFEEDBACKSHADER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/rendering/opengl/Shader.h"

/**
 * @brief Wrapper for OpenGL Shader programs that support Transform Feedback.
 *
 * Extends the base Shader class by enabling Transform Feedback, a mechanism
 * to capture output values from the Vertex (or Geometry) Shader before Rasterization.
 */
class TransformFeedbackShader : public Shader {
private:
    std::vector<std::string> m_varyings;

    TransformFeedbackShader(
        const std::unordered_map<unsigned int, std::string>& shaderTypeAndSource,
        const std::vector<std::string>& varyings,
        const ShaderDefines& defines
    );

public:
    /**
     * @brief Constructs a new Shader from a file path, defines captured varying outputs and optional preprocessor
     * definitions.
     *
     * @param shaderPath Path to the shader files directory.
     * @param varyings List of varying variable names to capture (Order matters).
     * @param defines Optional list of preprocessor macro definitions.
     */
    static TransformFeedbackShader create(
        const std::string& vertexShaderCode,
        const std::vector<std::string>& varyings,
        const ShaderDefines& defines = ShaderDefines()
    );

    TransformFeedbackShader() noexcept = default;
    TransformFeedbackShader(TransformFeedbackShader&& other) noexcept;
    virtual ~TransformFeedbackShader() = default;

    /** @return Reference to the list of captured varying out variable names. */
    inline const std::vector<std::string>& getVaryings() const { return m_varyings; }

    TransformFeedbackShader& operator=(TransformFeedbackShader&& other) noexcept;
};

#endif
