#ifndef TOOMANYBLOCKS_SHADER_H
#define TOOMANYBLOCKS_SHADER_H

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/rendering/lowlevelapi/RenderApiObject.h"
#include "engine/rendering/lowlevelapi/ShaderStorageBuffer.h"
#include "engine/rendering/lowlevelapi/UniformBuffer.h"

class ShaderDefines {
private:
    std::unordered_map<std::string, std::string> m_defines;

public:
    ShaderDefines() = default;

    inline ShaderDefines(const std::unordered_map<std::string, std::string>& defines) : m_defines(defines) {}

    inline void add(const std::string& key, const std::string& value = "") { m_defines.emplace(key, value); }

    inline const std::unordered_map<std::string, std::string>& definitions() const { return m_defines; }
};

/**
 * @brief Wrapper for OpenGL Shader programs.
 *
 * Exposes setters for uniform variables and UBOs and SSBOs.
 */
class Shader : public RenderApiObject {
private:
    std::vector<unsigned int> m_attachedShaders;  // Just holds shader references until link() is called

protected:
    struct BlockBindInfo {
        unsigned int blockIndex;
        unsigned int bindingPoint;
    };

    static thread_local unsigned int currentlyUsedShader;
    unsigned int m_nextUBOBindingPoint;
    unsigned int m_nextSSBOBindingPoint;
    std::unordered_map<std::string, int> m_uniformLocationCache;
    std::unordered_map<std::string, BlockBindInfo> m_uboBindingCache;
    std::unordered_map<std::string, BlockBindInfo> m_ssboBindingCache;

    /**
     * @brief Retrieves the location of a uniform variable by name.
     *
     * @param name The name of the uniform variable.
     * @return The location of the uniform, or -1 if not found (Is cached).
     */
    int getUniformLocation(const std::string& name);
    /**
     * @brief Retrieves index and binding point of a uniform block (UBO) by name.
     *
     * @param name The name of the uniform block in the shader.
     * @return Struct containing the block index and assigned binding point (Is cached).
     */
    BlockBindInfo getUBOBindInfo(const std::string& name);
    /**
     * @brief Retrieves index and binding point of a shader storage block (SSBO) by name.
     *
     * @param name The name of the shader storage block in the shader.
     * @return Struct containing the block index and assigned binding point (Is cached).
     */
    BlockBindInfo getSSBOBindInfo(const std::string& name);

    /**
     * @brief Final initilization step, internally links the program and cleans up shader refrences.
     */
    void link();

    Shader(
        const std::unordered_map<unsigned int, std::string>& shaderTypeAndSource,
        const ShaderDefines& defines,
        bool deferLinking = false  // Wether to link or not
    );

public:
    /**
     * @brief Sets the currently used shader on the current thread to 0.
     */
    static void useDefault();
    /**
     * @brief Synchronizes the wrapper's internal usage state with OpenGL.
     *
     * Should be used if the Shader usage is changed manually.
     */
    static void syncUsage();

    /**
     * @brief Constructs a new Shader from a file path and optional preprocessor definitions.
     *
     * @param shaderPath Path to the shader files directory.
     * @param defines Optional list of preprocessor macro definitions.
     */
    static Shader create(
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath,
        const ShaderDefines& defines = ShaderDefines()
    );

    Shader() noexcept : m_nextUBOBindingPoint(0), m_nextSSBOBindingPoint(0) {}
    Shader(Shader&& other) noexcept;
    virtual ~Shader();

    /**
     * @brief Sets this shader as the currently used program.
     *
     * @throws std::runtime_error If the buffer ID is 0 (uninitialized or moved-from).
     */
    void use() const;

    void bindUniformBuffer(const std::string& name, const UniformBuffer& ubo);

    void bindShaderStorageBuffer(const std::string& name, const ShaderStorageBuffer& ssbo);

    void setUniform(const std::string& name, bool value);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, unsigned int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const glm::bvec2& vector);
    void setUniform(const std::string& name, const glm::bvec3& vector);
    void setUniform(const std::string& name, const glm::bvec4& vector);
    void setUniform(const std::string& name, const glm::vec2& vector);
    void setUniform(const std::string& name, const glm::vec3& vector);
    void setUniform(const std::string& name, const glm::vec4& vector);
    void setUniform(const std::string& name, const glm::ivec2& vector);
    void setUniform(const std::string& name, const glm::ivec3& vector);
    void setUniform(const std::string& name, const glm::ivec4& vector);
    void setUniform(const std::string& name, const glm::uvec2& vector);
    void setUniform(const std::string& name, const glm::uvec3& vector);
    void setUniform(const std::string& name, const glm::uvec4& vector);
    void setUniform(const std::string& name, const glm::mat2& matrix);
    void setUniform(const std::string& name, const glm::mat3& matrix);
    void setUniform(const std::string& name, const glm::mat4& matrix);
    // No bool vec arrays cause the would have to be converted to int array (to have int pointer, wich opengl expects)
    void setUniform(const std::string& name, const int* values, size_t count);
    void setUniform(const std::string& name, const unsigned int* values, size_t count);
    void setUniform(const std::string& name, const float* values, size_t count);
    void setUniform(const std::string& name, const glm::vec2* vectors, size_t count);
    void setUniform(const std::string& name, const glm::vec3* vectors, size_t count);
    void setUniform(const std::string& name, const glm::vec4* vectors, size_t count);
    void setUniform(const std::string& name, const glm::ivec2* vectors, size_t count);
    void setUniform(const std::string& name, const glm::ivec3* vectors, size_t count);
    void setUniform(const std::string& name, const glm::ivec4* vectors, size_t count);
    void setUniform(const std::string& name, const glm::uvec2* vectors, size_t count);
    void setUniform(const std::string& name, const glm::uvec3* vectors, size_t count);
    void setUniform(const std::string& name, const glm::uvec4* vectors, size_t count);
    void setUniform(const std::string& name, const glm::mat2* matrices, size_t count);
    void setUniform(const std::string& name, const glm::mat3* matrices, size_t count);
    void setUniform(const std::string& name, const glm::mat4* matrices, size_t count);

    Shader& operator=(Shader&& other) noexcept;
};

#endif