#include "Shader.h"

#include <GL/glew.h>
#include <stddef.h>

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <stdexcept>

#include "Logger.h"
#include "engine/rendering/GLUtils.h"
#include "foundation/util/Utility.h"

thread_local unsigned int Shader::currentlyUsedShader = 0;

static unsigned int compileShader(unsigned int type, const std::string& source, const ShaderDefines& definitions) {
    const char* src = nullptr;
    std::string processedSource;

    // Insert optional compile definitions
    if (!definitions.definitions().empty()) {
        size_t versionPos = source.find("#version");
        size_t insertPos = source.find('\n', versionPos);

        if (versionPos != std::string::npos && insertPos != std::string::npos) {
            std::string defineBlock;
            for (const auto& define : definitions.definitions()) {
                defineBlock += "#define " + define.first;
                if (!define.second.empty()) {
                    defineBlock += " " + define.second;
                }
                defineBlock += "\n";
            }

            size_t preLength = insertPos + 1;                             // include newline
            processedSource.reserve(defineBlock.size() + source.size());  // Preallocate

            // Build final string in-place
            processedSource.append(source, 0, preLength);
            processedSource.append(defineBlock);
            processedSource.append(source, preLength);

            src = processedSource.c_str();
        } else {
            // Fallback
            src = source.c_str();
        }
    } else {
        src = source.c_str();
    }

    unsigned int id = glCreateShader(type);
    GLCALL(glShaderSource(id, 1, &src, nullptr));
    GLCALL(glCompileShader(id));

    int compileSuccess;
    GLCALL(glGetShaderiv(id, GL_COMPILE_STATUS, &compileSuccess));
    if (compileSuccess == GL_FALSE) {
        int length;
        GLCALL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        char* message = new char[length];
        GLCALL(glGetShaderInfoLog(id, length, &length, message));
        std::ostringstream ostream;
        ostream << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!"
                << std::endl;
        ostream << message << std::endl;
        lgr::lout.error(ostream.str());
        delete[] message;
        GLCALL(glDeleteShader(id));
        return 0;
    }

    return id;
}

int Shader::getUniformLocation(const std::string& name) {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    GLCALL(int location = glGetUniformLocation(m_rendererId, name.c_str()));
    if (location == -1) {  // -1 is not found / can happen for unused uniforms also
        lgr::lout.warn("Warning: location of '" + std::string(name) + "' uniform was not found!");
    }

    m_uniformLocationCache[name] = location;
    return location;
}

Shader::BlockBindInfo Shader::getUBOBindInfo(const std::string& name) {
    auto it = m_uboBindingCache.find(name);
    if (it != m_uboBindingCache.end()) {
        return it->second;
    }

    GLCALL(unsigned int blockIndex = glGetUniformBlockIndex(m_rendererId, name.c_str()));
    if (blockIndex == GL_INVALID_INDEX) {
        lgr::lout.warn("Warning: Index of '" + std::string(name) + "' uniform buffer was invalid!");
    }

    // Dymanically assign next available binding point, caller does not need to manage this.
    BlockBindInfo info = {blockIndex, m_nextUBOBindingPoint++};  // Binding point space of ubos differs from ssbos
    m_uboBindingCache[name] = info;
    return info;
}

Shader::BlockBindInfo Shader::getSSBOBindInfo(const std::string& name) {
    auto it = m_ssboBindingCache.find(name);
    if (it != m_ssboBindingCache.end()) {
        return it->second;
    }

    GLCALL(unsigned int blockIndex = glGetProgramResourceIndex(m_rendererId, GL_SHADER_STORAGE_BLOCK, name.c_str()));
    if (blockIndex == GL_INVALID_INDEX) {
        lgr::lout.warn("Warning: Index of '" + std::string(name) + "' uniform buffer was invalid!");
    }

    // Dymanically assign next available binding point, caller does not need to manage this.
    BlockBindInfo info = {blockIndex, m_nextSSBOBindingPoint++};  // Binding point space of ubos differs from ssbos
    m_ssboBindingCache[name] = info;
    return info;
}

void Shader::link() {
    GLCALL(glLinkProgram(m_rendererId));

    int linkingSuccess;
    GLCALL(glGetProgramiv(m_rendererId, GL_LINK_STATUS, &linkingSuccess));
    if (!linkingSuccess) {
        int logLength = 0;
        GLCALL(glGetProgramiv(m_rendererId, GL_INFO_LOG_LENGTH, &logLength));
        if (logLength > 0) {
            std::ostringstream ostream;
            char* infoLog = new char[logLength];
            glGetProgramInfoLog(m_rendererId, logLength, nullptr, infoLog);
            ostream << "Shader Linking Error:\n" << infoLog << std::endl;
            lgr::lout.error(ostream.str());
            delete[] infoLog;
        }
    }

    GLCALL(glValidateProgram(m_rendererId));

    // Cleanup shaders
    for (unsigned int shader : m_attachedShaders) {
        GLCALL(glDeleteShader(shader));
    }
    m_attachedShaders.clear();
}

Shader::Shader(
    const std::unordered_map<unsigned int, std::string>& shaderTypeAndSource,
    const ShaderDefines& definitions,
    bool deferLinking
)
    : m_nextUBOBindingPoint(0), m_nextSSBOBindingPoint(0) {
    GLCALL(m_rendererId = glCreateProgram());

    for (const auto& entry : shaderTypeAndSource) {
        unsigned int shader = compileShader(entry.first, entry.second, definitions);
        m_attachedShaders.push_back(shader);
        GLCALL(glAttachShader(m_rendererId, shader));
    }

    if (!deferLinking) {
        link();
    }
}

void Shader::useDefault() {
    if (Shader::currentlyUsedShader != 0) {
        GLCALL(glUseProgram(0));
        Shader::currentlyUsedShader = 0;
    }
}

void Shader::syncUsage() {
    int binding;
    GLCALL(glGetIntegerv(GL_CURRENT_PROGRAM, &binding));
    Shader::currentlyUsedShader = static_cast<unsigned int>(binding);
}

Shader Shader::create(
    const std::string& vertexShaderCode,
    const std::string& fragmentShaderCode,
    const ShaderDefines& defines
) {
    return Shader({{GL_VERTEX_SHADER, vertexShaderCode}, {GL_FRAGMENT_SHADER, fragmentShaderCode}}, defines);
}

Shader::Shader(Shader&& other) noexcept
    : RenderApiObject(std::move(other)),
      m_nextUBOBindingPoint(other.m_nextUBOBindingPoint),
      m_nextSSBOBindingPoint(other.m_nextSSBOBindingPoint),
      m_uniformLocationCache(std::move(other.m_uniformLocationCache)),
      m_uboBindingCache(std::move(other.m_uboBindingCache)),
      m_ssboBindingCache(std::move(other.m_ssboBindingCache)) {}

Shader::~Shader() {
    if (isValid()) {
        try {
            if (Shader::currentlyUsedShader == m_rendererId) {
                GLCALL(glUseProgram(0));
                Shader::currentlyUsedShader = 0;
            }
            GLCALL(glDeleteProgram(m_rendererId));
        } catch (const std::exception&) {
            lgr::lout.error("Error during Shader cleanup");
        }
    }
}

void Shader::use() const {
    if (!isValid()) throw std::runtime_error("Invalid state of Shader with id 0");

    if (Shader::currentlyUsedShader != m_rendererId) {
        GLCALL(glUseProgram(m_rendererId));
        Shader::currentlyUsedShader = m_rendererId;
    }
}

void Shader::bindUniformBuffer(const std::string& name, const UniformBuffer& ubo) {
    use();
    BlockBindInfo bindInfo = getUBOBindInfo(name);
    ubo.assignTo(bindInfo.bindingPoint);
    GLCALL(glUniformBlockBinding(m_rendererId, bindInfo.blockIndex, bindInfo.bindingPoint));
}

void Shader::bindShaderStorageBuffer(const std::string& name, const ShaderStorageBuffer& ssbo) {
    use();
    BlockBindInfo bindInfo = getSSBOBindInfo(name);
    ssbo.assignTo(bindInfo.bindingPoint);
    GLCALL(glShaderStorageBlockBinding(m_rendererId, bindInfo.blockIndex, bindInfo.bindingPoint));
}

void Shader::setUniform(const std::string& name, bool value) {
    use();
    GLCALL(glUniform1i(getUniformLocation(name), value));
}

void Shader::setUniform(const std::string& name, int value) {
    use();
    GLCALL(glUniform1i(getUniformLocation(name), value));
}

void Shader::setUniform(const std::string& name, unsigned int value) {
    use();
    GLCALL(glUniform1ui(getUniformLocation(name), value));
}

void Shader::setUniform(const std::string& name, float value) {
    use();
    GLCALL(glUniform1f(getUniformLocation(name), value));
}

void Shader::setUniform(const std::string& name, const glm::bvec2& vector) {
    use();
    GLCALL(glUniform2i(getUniformLocation(name), vector.x, vector.y));
}

void Shader::setUniform(const std::string& name, const glm::bvec3& vector) {
    use();
    GLCALL(glUniform3i(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void Shader::setUniform(const std::string& name, const glm::bvec4& vector) {
    use();
    GLCALL(glUniform4i(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::setUniform(const std::string& name, const glm::vec2& vector) {
    use();
    GLCALL(glUniform2f(getUniformLocation(name), vector.x, vector.y));
}

void Shader::setUniform(const std::string& name, const glm::vec3& vector) {
    use();
    GLCALL(glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void Shader::setUniform(const std::string& name, const glm::vec4& vector) {
    use();
    GLCALL(glUniform4f(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::setUniform(const std::string& name, const glm::ivec2& vector) {
    use();
    GLCALL(glUniform2i(getUniformLocation(name), vector.x, vector.y));
}

void Shader::setUniform(const std::string& name, const glm::ivec3& vector) {
    use();
    GLCALL(glUniform3i(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void Shader::setUniform(const std::string& name, const glm::ivec4& vector) {
    use();
    GLCALL(glUniform4i(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::setUniform(const std::string& name, const glm::uvec2& vector) {
    use();
    GLCALL(glUniform2ui(getUniformLocation(name), vector.x, vector.y));
}

void Shader::setUniform(const std::string& name, const glm::uvec3& vector) {
    use();
    GLCALL(glUniform3ui(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void Shader::setUniform(const std::string& name, const glm::uvec4& vector) {
    use();
    GLCALL(glUniform4ui(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::setUniform(const std::string& name, const glm::mat2& matrix) {
    use();
    GLCALL(glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setUniform(const std::string& name, const glm::mat3& matrix) {
    use();
    GLCALL(glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setUniform(const std::string& name, const glm::mat4& matrix) {
    use();
    GLCALL(glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setUniform(const std::string& name, const int* values, size_t count) {
    use();
    GLCALL(glUniform1iv(getUniformLocation(name), count, values));
}

void Shader::setUniform(const std::string& name, const unsigned int* values, size_t count) {
    use();
    GLCALL(glUniform1uiv(getUniformLocation(name), count, values));
}

void Shader::setUniform(const std::string& name, const float* values, size_t count) {
    use();
    GLCALL(glUniform1fv(getUniformLocation(name), count, values));
}

void Shader::setUniform(const std::string& name, const glm::vec2* vectors, size_t count) {
    use();
    GLCALL(glUniform2fv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::vec3* vectors, size_t count) {
    use();
    GLCALL(glUniform3fv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::vec4* vectors, size_t count) {
    use();
    GLCALL(glUniform4fv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::ivec2* vectors, size_t count) {
    use();
    GLCALL(glUniform2iv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::ivec3* vectors, size_t count) {
    use();
    GLCALL(glUniform3iv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::ivec4* vectors, size_t count) {
    use();
    GLCALL(glUniform4iv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::uvec2* vectors, size_t count) {
    use();
    GLCALL(glUniform2uiv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::uvec3* vectors, size_t count) {
    use();
    GLCALL(glUniform3uiv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::uvec4* vectors, size_t count) {
    use();
    GLCALL(glUniform4uiv(getUniformLocation(name), count, glm::value_ptr(vectors[0])));
}

void Shader::setUniform(const std::string& name, const glm::mat2* matrices, size_t count) {
    use();
    GLCALL(glUniformMatrix2fv(getUniformLocation(name), count, GL_FALSE, glm::value_ptr(matrices[0])));
}

void Shader::setUniform(const std::string& name, const glm::mat3* matrices, size_t count) {
    use();
    GLCALL(glUniformMatrix3fv(getUniformLocation(name), count, GL_FALSE, glm::value_ptr(matrices[0])));
}

void Shader::setUniform(const std::string& name, const glm::mat4* matrices, size_t count) {
    use();
    GLCALL(glUniformMatrix4fv(getUniformLocation(name), count, GL_FALSE, glm::value_ptr(matrices[0])));
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_rendererId != 0) {
            try {
                if (Shader::currentlyUsedShader == m_rendererId) {
                    GLCALL(glUseProgram(0));
                    Shader::currentlyUsedShader = 0;
                }
                GLCALL(glDeleteProgram(m_rendererId));
            } catch (const std::exception&) {
                lgr::lout.error("Error during Shader cleanup");
            }
        }
        RenderApiObject::operator=(std::move(other));

        m_nextUBOBindingPoint = other.m_nextUBOBindingPoint;
        m_nextSSBOBindingPoint = other.m_nextSSBOBindingPoint;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        m_uboBindingCache = std::move(other.m_uboBindingCache);
        m_ssboBindingCache = std::move(other.m_ssboBindingCache);
    }
    return *this;
}