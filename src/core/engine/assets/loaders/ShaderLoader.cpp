#include "ShaderLoader.h"

#include <cstring>
#include <stdexcept>
#include <unordered_set>

#include "foundation/util/Utility.h"

static void loadShaderWithIncludeProcessing(
    const std::filesystem::path& filePath,
    std::unordered_set<std::string>& includedFiles,
    std::string& output
) {
    std::filesystem::path path = filePath.lexically_normal();
    std::string normalizedPath = path.string();

    if (!includedFiles.insert(normalizedPath).second) {
        return;
    }

    const std::filesystem::path parentPath = path.parent_path();
    const std::string source = readFile(normalizedPath);

    const char* begin = source.data();
    const char* end = begin + source.size();
    const char* cursor = begin;
    const char* outputStart = begin;

    while (cursor < end) {
        const char* lineEnd = static_cast<const char*>(std::memchr(cursor, '\n', end - cursor));
        if (!lineEnd) {
            lineEnd = end;
        }

        const char* directiveStart = cursor;
        while (directiveStart < lineEnd && (*directiveStart == ' ' || *directiveStart == '\t')) {
            directiveStart++;
        }

        constexpr size_t includeLength = 8;  // "#include"

        if (lineEnd - directiveStart >= includeLength && std::memcmp(directiveStart, "#include", includeLength) == 0) {
            const char* afterInclude = directiveStart + includeLength;

            // Make sure this is actually "#include".
            if (afterInclude < lineEnd && *afterInclude != ' ' && *afterInclude != '\t') {
                directiveStart = lineEnd < end ? lineEnd + 1 : end;
                continue;
            }

            // Find quotes
            const char* quoteStart = afterInclude;
            while (quoteStart < lineEnd && (*quoteStart == ' ' || *quoteStart == '\t')) {
                quoteStart++;
            }

            if (quoteStart >= lineEnd || *quoteStart != '"') {
                throw std::runtime_error("Malformed #include in " + normalizedPath);
            } else {
                quoteStart++;
            }

            const char* quoteEnd = static_cast<const char*>(std::memchr(quoteStart, '"', lineEnd - quoteStart));
            if (!quoteEnd) {
                throw std::runtime_error("Malformed #include in " + normalizedPath);
            }

            std::string filePath = std::string(quoteStart, quoteEnd - quoteStart);
            std::filesystem::path includePath = (parentPath / filePath).lexically_normal();

            // Copy source before the #include.
            output.append(outputStart, cursor - outputStart);

            // Insert includes in included file
            loadShaderWithIncludeProcessing(includePath, includedFiles, output);
            output.push_back('\n');

            outputStart = lineEnd < end ? lineEnd + 1 : end;
        }

        cursor = lineEnd < end ? lineEnd + 1 : end;
    }

    output.append(outputStart, end - outputStart);
}

static std::string loadShaderSource(const std::filesystem::path& filePath) {
    std::string output;
    std::unordered_set<std::string> includedFiles;
    loadShaderWithIncludeProcessing(filePath, includedFiles, output);
    return output;
}

CPUShader loadShaderFromFile(const std::string& shaderPath, ShaderLoadOption option) {
    CPUShader shader;

    size_t pos = shaderPath.find_last_of("/\\");
    std::string basename = shaderPath;
    if (pos != std::string::npos) {
        basename = shaderPath.substr(pos + 1);
    }

    if (option == ShaderLoadOption::VertexAndFragment || option == ShaderLoadOption::VertexOnly) {
        std::string vertFile = shaderPath + "/" + basename + ".vert";
        shader.vertexShader = loadShaderSource(vertFile);
    }
    if (option == ShaderLoadOption::VertexAndFragment || option == ShaderLoadOption::FragmentOnly) {
        std::string fragFile = shaderPath + "/" + basename + ".frag";
        shader.fragmentShader = loadShaderSource(fragFile);
    }

    return shader;
}