#ifndef TOOMANYBLOCKS_SHADERGENERATOR_H
#define TOOMANYBLOCKS_SHADERGENERATOR_H

#include <array>
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "engine/assets/cpu/CPUShader.h"
#include "engine/rendering/shaders/generator/Glsl.h"
#include "engine/rendering/shaders/generator/ShaderExpr.h"
#include "engine/rendering/shaders/ShaderInterface.h"
#include "engine/rendering/shaders/generator/modules/ShaderModuleRegistry.h"

struct ShaderContract {
    CPUShader cpuShader;
    ShaderInterface interface;
};

class ShaderGenerator {
    friend class ShaderExpr;

private:
    enum class ValueOperationType {
        Assign,
        Call,
        VoidCall,  // Statement root
        Binary,
        Unary,
        Swizzle,
        Varying,
    };

    enum class BinaryOperator {
        Add,
        Subtract,
        Multiply,
        Divide,
    };

    enum class UnaryOperator {
        Negate,
    };

    enum class ValueKind {
        VertexInput,
        VertexOutput,    // Value root
        VertexPosition,  // Value root
        Uniform,
        Constant,
        Local,
        FragmentOutput,  // Value root
        Varying,
    };

    struct ValueOperation {
        ValueOperationType type;

        unsigned int output;
        std::vector<unsigned int> inputs;

        std::string function;

        ValueScope scope = ValueScope::Vertex;

        BinaryOperator binaryOperator = BinaryOperator::Add;
        UnaryOperator unaryOperator = UnaryOperator::Negate;

        std::string swizzle;
    };

    struct ShaderValue {
        GlslType type;
        ValueKind kind;
        ShaderAvailability availability;

        std::string name;

        unsigned int location;

        VaryingInterpolation interpolation = VaryingInterpolation::Smooth;

        // Used by Constant.
        std::string literal;

        // Operation that initially created this value.
        int initialProducerOperation = -1;
    };

    struct ShaderReachability {
        // [value][stage]
        std::vector<std::array<bool, 2>> values;

        // [operation][stage]
        std::vector<std::array<bool, 2>> operations;
    };

    const ShaderModuleRegistry* m_modules;
    std::vector<ShaderValue> m_values;
    std::vector<ValueOperation> m_operations;

    unsigned int createValue(GlslType type, ValueKind kind, ShaderAvailability availability);

    const ShaderValue& getValue(unsigned int id) const;
    ShaderValue& getValue(unsigned int id);
    std::string getValueName(unsigned int id) const;

    void validateInput(const ShaderValue& input) const;
    void validateOutput(const ShaderValue& output) const;
    void validateAssignment(unsigned int output, unsigned int input) const;
    void validateCall(unsigned int output, const std::string& function, const std::vector<unsigned int>& inputs) const;
    void validateVoidCall(ValueScope scope, const std::string& function, const std::vector<unsigned int>& inputs) const;
    void validateBinary(BinaryOperator op, const ShaderValue& lhs, const ShaderValue& rhs) const;
    void validateUnary(UnaryOperator op, const ShaderValue& input) const;
    void validateSwizzle(const ShaderValue& input, const std::string& swizzle) const;
    void validateLocation(unsigned int location, ValueKind kind) const;

    GlslType resolveBinaryResultType(BinaryOperator op, GlslType lhs, GlslType rhs) const;
    GlslType resolveSwizzleResultType(GlslType inputType, size_t componentCount) const;
    ShaderAvailability resolveExpressionAvailability(const std::vector<unsigned int>& inputs) const;

    bool definesValue(const ValueOperation& operation) const;
    int findDefinition(unsigned int value, unsigned int beforeOperation, ValueScope scope) const;
    ShaderReachability analyzeReachability() const;

    void createAssignment(unsigned int output, unsigned int input);
    ShaderExpr createBinary(BinaryOperator op, unsigned int lhs, unsigned int rhs);
    ShaderExpr createUnary(UnaryOperator op, unsigned int input);
    ShaderExpr createSwizzle(unsigned int input, std::string swizzle);
    ShaderExpr createConstant(GlslType type, std::string literal);

    void emitVertexInputs(
        std::ostringstream& vertex,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;
    void emitVertexOutputs(
        std::ostringstream& vertex,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;
    void emitFragmentOutputs(
        std::ostringstream& fragment,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;
    void emitUniforms(
        std::ostringstream& vertex,
        std::ostringstream& fragment,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;
    void emitVaryings(
        std::ostringstream& vertex,
        std::ostringstream& fragment,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;

    void emitLocals(std::ostringstream& out, ValueScope scope, const ShaderReachability& reachable) const;
    void emitOperation(std::ostringstream& out, unsigned int operationId, const ValueOperation& operation) const;
    void emitOperations(std::ostringstream& out, ValueScope scope, const ShaderReachability& reachable) const;

    void emitAssign(std::ostringstream& out, const ValueOperation& operation) const;
    void emitCall(std::ostringstream& out, const ValueOperation& operation) const;
    void emitVoidCall(std::ostringstream& out, const ValueOperation& operation) const;
    void emitBinary(std::ostringstream& out, const ValueOperation& operation) const;
    void emitUnary(std::ostringstream& out, const ValueOperation& operation) const;
    void emitSwizzle(std::ostringstream& out, const ValueOperation& operation) const;
    void emitVarying(std::ostringstream& out, const ValueOperation& operation) const;

    void collectModules(
        std::ostringstream& out,
        ValueScope scope,
        ShaderContract& contract,
        const ShaderReachability& reachable
    ) const;

public:
    ShaderGenerator(const ShaderModuleRegistry* modules = nullptr);

    ShaderExpr local(GlslType type, ValueScope scope);
    ShaderExpr uniform(GlslType type, std::string name);
    ShaderExpr vertexInput(unsigned int location, GlslType type);
    ShaderExpr vertexPosition();
    ShaderExpr vertexOutput(unsigned int location, GlslType type);
    ShaderExpr fragmentOutput(unsigned int location, GlslType type);

    ShaderExpr constant(bool value);
    ShaderExpr constant(int value);
    ShaderExpr constant(unsigned int value);
    ShaderExpr constant(float value);
    ShaderExpr constant(const glm::bvec2& value);
    ShaderExpr constant(const glm::bvec3& value);
    ShaderExpr constant(const glm::bvec4& value);
    ShaderExpr constant(const glm::ivec2& value);
    ShaderExpr constant(const glm::ivec3& value);
    ShaderExpr constant(const glm::ivec4& value);
    ShaderExpr constant(const glm::uvec2& value);
    ShaderExpr constant(const glm::uvec3& value);
    ShaderExpr constant(const glm::uvec4& value);
    ShaderExpr constant(const glm::vec2& value);
    ShaderExpr constant(const glm::vec3& value);
    ShaderExpr constant(const glm::vec4& value);

    // Explicit vertex -> fragment interface.
    ShaderExpr varying(ShaderExpr input, VaryingInterpolation interpolation = VaryingInterpolation::Smooth);

    // Statements
    void call(ValueScope scope, std::string function, std::vector<ShaderExpr> inputs);
    ShaderExpr call(GlslType returnType, std::string function, std::vector<ShaderExpr> inputs);

    inline const std::vector<ShaderValue>& values() const { return m_values; }
    inline const std::vector<ValueOperation>& operations() const { return m_operations; }

    ShaderContract generate() const;
};

#endif
