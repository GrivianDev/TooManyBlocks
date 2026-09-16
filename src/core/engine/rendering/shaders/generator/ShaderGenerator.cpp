#include "ShaderGenerator.h"

#include <array>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <unordered_set>

#include "Logger.h"

static constexpr const char* GLSL_VERSION = "#version 430 core";
static constexpr const char* GLSL_INDENT = "    ";

static std::string formatFloat(float value) {
    std::ostringstream stream;
    stream << std::setprecision(9) << value;

    std::string result = stream.str();

    if (result.find('.') == std::string::npos && result.find('e') == std::string::npos &&
        result.find('E') == std::string::npos) {
        result += ".0";
    }

    return result;
}

static bool isAdditiveType(GlslType type) {
    switch (type) {
        case GlslType::Float:
        case GlslType::Vec2:
        case GlslType::Vec3:
        case GlslType::Vec4:

        case GlslType::Int:
        case GlslType::IVec2:
        case GlslType::IVec3:
        case GlslType::IVec4:

        case GlslType::UInt:
        case GlslType::UVec2:
        case GlslType::UVec3:
        case GlslType::UVec4:

        case GlslType::Mat2:
        case GlslType::Mat3:
        case GlslType::Mat4: return true;

        default: return false;
    }
}

static std::string glslTypeToString(GlslType type) {
    switch (type) {
        case GlslType::Float: return "float";
        case GlslType::Vec2: return "vec2";
        case GlslType::Vec3: return "vec3";
        case GlslType::Vec4: return "vec4";

        case GlslType::Int: return "int";
        case GlslType::IVec2: return "ivec2";
        case GlslType::IVec3: return "ivec3";
        case GlslType::IVec4: return "ivec4";

        case GlslType::UInt: return "uint";
        case GlslType::UVec2: return "uvec2";
        case GlslType::UVec3: return "uvec3";
        case GlslType::UVec4: return "uvec4";

        case GlslType::Bool: return "bool";
        case GlslType::BVec2: return "bvec2";
        case GlslType::BVec3: return "bvec3";
        case GlslType::BVec4: return "bvec4";

        case GlslType::Mat2: return "mat2";
        case GlslType::Mat3: return "mat3";
        case GlslType::Mat4: return "mat4";

        case GlslType::Sampler2D: return "sampler2D";
        case GlslType::Sampler3D: return "sampler3D";
        case GlslType::SamplerCube: return "samplerCube";
        case GlslType::Sampler2DArray: return "sampler2DArray";

        case GlslType::ISampler2D: return "isampler2D";
        case GlslType::ISampler3D: return "isampler3D";
        case GlslType::ISamplerCube: return "isamplerCube";
        case GlslType::ISampler2DArray: return "isampler2DArray";

        case GlslType::USampler2D: return "usampler2D";
        case GlslType::USampler3D: return "usampler3D";
        case GlslType::USamplerCube: return "usamplerCube";
        case GlslType::USampler2DArray: return "usampler2DArray";

        case GlslType::Sampler2DShadow: return "sampler2DShadow";
        case GlslType::SamplerCubeShadow: return "samplerCubeShadow";
        case GlslType::Sampler2DArrayShadow: return "sampler2DArrayShadow";

        default: throw std::runtime_error("ShaderGenerator: unknown GlslType");
    }
}

static constexpr size_t stageIndex(ValueScope scope) { return scope == ValueScope::Vertex ? 0 : 1; }

unsigned int ShaderGenerator::createValue(GlslType type, ValueKind kind, ShaderAvailability availability) {
    if (!availability.any()) {
        throw std::runtime_error("ShaderGenerator: expression has no valid shader stage");
    }

    unsigned int id = static_cast<unsigned int>(m_values.size());

    ShaderValue value{};
    value.type = type;
    value.kind = kind;
    value.availability = availability;
    if (kind == ValueKind::Local) {
        value.name = "_l" + std::to_string(id);
    }

    m_values.push_back(std::move(value));

    return id;
}

const ShaderGenerator::ShaderValue& ShaderGenerator::getValue(unsigned int id) const {
    if (id >= m_values.size()) {
        throw std::runtime_error("ShaderGenerator: invalid ValueId " + std::to_string(id));
    }
    return m_values[id];
}

ShaderGenerator::ShaderValue& ShaderGenerator::getValue(unsigned int id) {
    if (id >= m_values.size()) {
        throw std::runtime_error("ShaderGenerator: invalid ValueId " + std::to_string(id));
    }
    return m_values[id];
}

std::string ShaderGenerator::getValueName(unsigned int id) const {
    const ShaderValue& value = getValue(id);

    switch (value.kind) {
        case ValueKind::VertexInput:
        case ValueKind::VertexOutput:
        case ValueKind::Uniform:
        case ValueKind::Local:
        case ValueKind::FragmentOutput:
        case ValueKind::VertexPosition:
        case ValueKind::Varying: return value.name;
        case ValueKind::Constant: return value.literal;
        default: throw std::runtime_error("Can not get name for unhandled value type");
    }
}

void ShaderGenerator::validateInput(const ShaderValue& input) const {
    switch (input.kind) {
        case ValueKind::VertexInput:
        case ValueKind::Uniform:
        case ValueKind::Constant:
        case ValueKind::Local:
        case ValueKind::Varying: return;
        case ValueKind::VertexPosition:
            throw std::runtime_error("ShaderGenerator: gl_Position cannot be used as an input");
        case ValueKind::VertexOutput:
            throw std::runtime_error("ShaderGenerator: vertex outputs cannot be used as inputs");
        case ValueKind::FragmentOutput:
            throw std::runtime_error("ShaderGenerator: fragment outputs cannot be used as inputs");
        default: throw std::runtime_error("ShaderGenerator: invalid shader value kind");
    }
}

void ShaderGenerator::validateOutput(const ShaderValue& output) const {
    switch (output.kind) {
        case ValueKind::Local:
        case ValueKind::VertexOutput:
        case ValueKind::VertexPosition:
        case ValueKind::FragmentOutput: return;
        case ValueKind::VertexInput: throw std::runtime_error("ShaderGenerator: vertex inputs cannot be written to");
        case ValueKind::Uniform: throw std::runtime_error("ShaderGenerator: uniforms cannot be written to");
        case ValueKind::Constant: throw std::runtime_error("ShaderGenerator: constants cannot be written to");
        case ValueKind::Varying:
            throw std::runtime_error(
                "ShaderGenerator: varyings cannot be written with assign(); "
                "they are written automatically from their source expression"
            );
        default: throw std::runtime_error("ShaderGenerator: invalid shader value kind");
    }
}

void ShaderGenerator::validateAssignment(unsigned int output, unsigned int input) const {
    const ShaderValue& outputValue = getValue(output);
    const ShaderValue& inputValue = getValue(input);

    validateOutput(outputValue);
    validateInput(inputValue);

    if (outputValue.type != inputValue.type) {
        throw std::runtime_error("ShaderGenerator: assignment type mismatch");
    }
    if (outputValue.availability.vertex && !inputValue.availability.vertex) {
        throw std::runtime_error("ShaderGenerator: input is not available in the vertex shader");
    }
    if (outputValue.availability.fragment && !inputValue.availability.fragment) {
        throw std::runtime_error("ShaderGenerator: input is not available in the fragment shader");
    }
}

void ShaderGenerator::validateCall(
    unsigned int output,
    const std::string& function,
    const std::vector<unsigned int>& inputs
) const {
    if (function.empty()) {
        throw std::runtime_error("ShaderGenerator: function name cannot be empty");
    }

    const ShaderValue& outputValue = getValue(output);

    validateOutput(outputValue);

    for (unsigned int input : inputs) {
        validateInput(getValue(input));
    }

    if (!outputValue.availability.any()) {
        throw std::runtime_error("ShaderGenerator: call has no valid shader stage");
    }
}

void ShaderGenerator::validateVoidCall(
    ValueScope scope,
    const std::string& function,
    const std::vector<unsigned int>& inputs
) const {
    if (function.empty()) {
        throw std::runtime_error("ShaderGenerator: function name cannot be empty");
    }

    for (unsigned int input : inputs) {
        const ShaderValue& inputValue = getValue(input);

        validateInput(inputValue);

        if (!inputValue.availability.available(scope)) {
            throw std::runtime_error(
                "ShaderGenerator: call input is not available "
                "in the requested shader stage"
            );
        }
    }
}

void ShaderGenerator::validateBinary(BinaryOperator op, const ShaderValue& lhs, const ShaderValue& rhs) const {
    validateInput(lhs);
    validateInput(rhs);
}

void ShaderGenerator::validateUnary(UnaryOperator op, const ShaderValue& input) const {
    validateInput(input);

    switch (op) {
        case UnaryOperator::Negate:
            switch (input.type) {
                case GlslType::Float:
                case GlslType::Vec2:
                case GlslType::Vec3:
                case GlslType::Vec4:

                case GlslType::Int:
                case GlslType::IVec2:
                case GlslType::IVec3:
                case GlslType::IVec4:

                case GlslType::Mat4: return;

                case GlslType::UInt:
                case GlslType::UVec2:
                case GlslType::UVec3:
                case GlslType::UVec4: break;

                default: throw std::runtime_error("ShaderGenerator: unary negate is not valid for this type");
            }

        default: throw std::runtime_error("ShaderGenerator: invalid unary operator");
    }
}

void ShaderGenerator::validateSwizzle(const ShaderValue& input, const std::string& swizzle) const {
    validateInput(input);

    if (swizzle.empty() || swizzle.size() > 4) {
        throw std::runtime_error("ShaderGenerator: invalid swizzle expression");
    }

    unsigned int componentCount = 0;

    switch (input.type) {
        case GlslType::BVec2:
        case GlslType::Vec2:
        case GlslType::IVec2:
        case GlslType::UVec2: componentCount = 2; break;

        case GlslType::BVec3:
        case GlslType::Vec3:
        case GlslType::IVec3:
        case GlslType::UVec3: componentCount = 3; break;

        case GlslType::BVec4:
        case GlslType::Vec4:
        case GlslType::IVec4:
        case GlslType::UVec4: componentCount = 4; break;

        default: throw std::runtime_error("ShaderGenerator: swizzle requires a vector input");
    }

    bool xyzw = swizzle.find_first_of("xyzw") != std::string::npos;
    bool rgba = swizzle.find_first_of("rgba") != std::string::npos;
    if (xyzw && rgba) {
        throw std::runtime_error("ShaderGenerator: swizzle cannot mix xyzw and rgba components");
    }
    if (!xyzw && !rgba) {
        throw std::runtime_error("ShaderGenerator: invalid swizzle");
    }

    const char* allowed = xyzw ? "xyzw" : "rgba";

    for (char component : swizzle) {
        const char* found = std::find(allowed, allowed + 4, component);

        if (found == allowed + 4) {
            throw std::runtime_error("ShaderGenerator: invalid swizzle component");
        }

        unsigned int index = static_cast<unsigned int>(found - allowed);
        if (index >= componentCount) {
            throw std::runtime_error("ShaderGenerator: swizzle component out of range");
        }
    }
}

void ShaderGenerator::validateLocation(unsigned int location, ValueKind kind) const {
    for (const ShaderValue& value : m_values) {
        if (value.kind != kind) {
            continue;
        }

        if (value.location == location) {
            throw std::runtime_error("ShaderGenerator: duplicate shader location " + std::to_string(location));
        }
    }
}

GlslType ShaderGenerator::resolveBinaryResultType(BinaryOperator op, GlslType lhs, GlslType rhs) const {
    switch (op) {
        case BinaryOperator::Add:
        case BinaryOperator::Subtract: {
            if (lhs == rhs && isAdditiveType(lhs)) {
                return lhs;
            }
            throw std::runtime_error("ShaderGenerator: invalid binary operation types");
        }

        case BinaryOperator::Multiply: {
            // Same-type arithmetic.
            if (lhs == rhs) {
                switch (lhs) {
                    case GlslType::Float:
                    case GlslType::Vec2:
                    case GlslType::Vec3:
                    case GlslType::Vec4:
                    case GlslType::Int:
                    case GlslType::UInt:
                    case GlslType::IVec2:
                    case GlslType::IVec3:
                    case GlslType::IVec4:
                    case GlslType::UVec2:
                    case GlslType::UVec3:
                    case GlslType::UVec4:
                    case GlslType::Mat4: return lhs;
                }
            }
            // float * vector
            if (lhs == GlslType::Float) {
                switch (rhs) {
                    case GlslType::Vec2:
                    case GlslType::Vec3:
                    case GlslType::Vec4: return rhs;

                    default: break;
                }
            }
            // vector * float
            if (rhs == GlslType::Float) {
                switch (lhs) {
                    case GlslType::Vec2:
                    case GlslType::Vec3:
                    case GlslType::Vec4: return lhs;

                    default: break;
                }
            }
            // mat4 * vec4
            if (lhs == GlslType::Mat4 && rhs == GlslType::Vec4) {
                return GlslType::Vec4;
            }
            // vec4 * mat4
            if (lhs == GlslType::Vec4 && rhs == GlslType::Mat4) {
                return GlslType::Vec4;
            }
            // mat4 * mat4
            if (lhs == GlslType::Mat4 && rhs == GlslType::Mat4) {
                return GlslType::Mat4;
            }

            throw std::runtime_error("ShaderGenerator: invalid binary multiplication types");
        }

        case BinaryOperator::Divide: {
            // scalar / scalar
            if (lhs == GlslType::Float && rhs == GlslType::Float) {
                return GlslType::Float;
            }
            // vector / float
            if (rhs == GlslType::Float) {
                switch (lhs) {
                    case GlslType::Vec2:
                    case GlslType::Vec3:
                    case GlslType::Vec4:
                    case GlslType::Mat4: return lhs;

                    default: break;
                }
            }

            throw std::runtime_error("ShaderGenerator: invalid binary division types");
        }
    }

    throw std::runtime_error("ShaderGenerator: unhandled binary operator");
}

GlslType ShaderGenerator::resolveSwizzleResultType(GlslType inputType, size_t componentCount) const {
    switch (inputType) {
        case GlslType::Vec2:
        case GlslType::Vec3:
        case GlslType::Vec4:
            switch (componentCount) {
                case 1: return GlslType::Float;
                case 2: return GlslType::Vec2;
                case 3: return GlslType::Vec3;
                case 4: return GlslType::Vec4;
                default: break;
            }
            break;

        case GlslType::IVec2:
        case GlslType::IVec3:
        case GlslType::IVec4:
            switch (componentCount) {
                case 1: return GlslType::Int;
                case 2: return GlslType::IVec2;
                case 3: return GlslType::IVec3;
                case 4: return GlslType::IVec4;
                default: break;
            }
            break;

        case GlslType::UVec2:
        case GlslType::UVec3:
        case GlslType::UVec4:
            switch (componentCount) {
                case 1: return GlslType::UInt;
                case 2: return GlslType::UVec2;
                case 3: return GlslType::UVec3;
                case 4: return GlslType::UVec4;
                default: break;
            }
            break;

        case GlslType::BVec2:
        case GlslType::BVec3:
        case GlslType::BVec4:
            switch (componentCount) {
                case 1: return GlslType::Bool;
                case 2: return GlslType::BVec2;
                case 3: return GlslType::BVec3;
                case 4: return GlslType::BVec4;
                default: break;
            }
            break;

        default: break;
    }

    throw std::runtime_error("ShaderGenerator: invalid swizzle");
}

ShaderAvailability ShaderGenerator::resolveExpressionAvailability(const std::vector<unsigned int>& inputs) const {
    ShaderAvailability result{true, true};

    for (unsigned int id : inputs) {
        const ShaderValue& value = getValue(id);

        result.vertex &= value.availability.vertex;
        result.fragment &= value.availability.fragment;
    }

    if (!result.any()) {
        throw std::runtime_error("ShaderGenerator: expression has no valid shader stage");
    }

    return result;
}

bool ShaderGenerator::definesValue(const ValueOperation& operation) const {
    switch (operation.type) {
        case ValueOperationType::Assign:
        case ValueOperationType::Call:
        case ValueOperationType::Binary:
        case ValueOperationType::Unary:
        case ValueOperationType::Swizzle:
        case ValueOperationType::Varying: return true;

        case ValueOperationType::VoidCall: return false;
    }

    return false;
}

int ShaderGenerator::findDefinition(unsigned int value, unsigned int beforeOperation, ValueScope scope) const {
    for (int i = static_cast<int>(beforeOperation) - 1; i >= 0; i--) {
        const ValueOperation& operation = m_operations[i];

        if (!definesValue(operation)) {
            continue;
        }

        if (operation.output == value) {
            return i;
        }
    }

    return -1;
}

ShaderGenerator::ShaderReachability ShaderGenerator::analyzeReachability() const {
    ShaderReachability result;

    result.values.resize(m_values.size());
    result.operations.resize(m_operations.size());

    std::function<void(unsigned int, ValueScope, unsigned int)> markValue;
    std::function<void(unsigned int, ValueScope)> markOperation;

    markValue = [&](unsigned int valueId, ValueScope scope, unsigned int beforeOperation) {
        const ShaderValue& value = getValue(valueId);

        const size_t stage = stageIndex(scope);

        /*
         * Constants, uniforms and inputs have no producer operations, so if a live operation uses them, they become
         * live leaves of the dependency graph.
         */
        result.values[valueId][stage] = true;

        switch (value.kind) {
            case ValueKind::Constant:
            case ValueKind::Uniform:
            case ValueKind::VertexInput: return;
        }

        int definition = findDefinition(valueId, beforeOperation, scope);
        if (definition < 0) {
            /*
             * No definition operation means this is either:
             *
             * - an interface value whose declaration is enough
             * - an uninitialized local
             */
            return;
        }

        markOperation(definition, scope);
    };

    markOperation = [&](unsigned int operationIndex, ValueScope scope) {
        const ValueOperation& operation = m_operations[operationIndex];

        switch (operation.type) {
            case ValueOperationType::Assign:
            case ValueOperationType::Call:
            case ValueOperationType::Binary:
            case ValueOperationType::Unary:
            case ValueOperationType::Swizzle:
            case ValueOperationType::VoidCall: {
                const size_t stage = stageIndex(scope);

                if (result.operations[operationIndex][stage]) return;

                // Mark live and all of the operations inputs
                result.operations[operationIndex][stage] = true;
                for (unsigned int input : operation.inputs) {
                    markValue(input, scope, operationIndex);
                }

                break;
            }

            case ValueOperationType::Varying: {
                if (scope != ValueScope::Fragment) {
                    throw std::runtime_error("ShaderGenerator: varying can only be consumed from fragment stage");
                }

                /*
                 * The value feeding the varying is computed in the vertex shader.
                 */
                markValue(operation.inputs[0], ValueScope::Vertex, operationIndex);

                /*
                 * But the write operation exists only in the vertex shader.
                 */
                result.operations[operationIndex][stageIndex(ValueScope::Vertex)] = true;

                // The varying declaration exists in both stages.
                result.values[operation.output][stageIndex(ValueScope::Vertex)] = true;
                result.values[operation.output][stageIndex(ValueScope::Fragment)] = true;

                break;
            }
        }
    };

    // Value roots
    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        switch (value.kind) {
            case ValueKind::VertexPosition:
            case ValueKind::VertexOutput: markValue(id, ValueScope::Vertex, m_operations.size()); break;

            case ValueKind::FragmentOutput: markValue(id, ValueScope::Fragment, m_operations.size()); break;

            default: break;
        }
    }

    // VoidCall is an explicit side-effect root
    for (unsigned int id = 0; id < m_operations.size(); id++) {
        const ValueOperation& operation = m_operations[id];

        if (operation.type == ValueOperationType::VoidCall) {
            markOperation(id, operation.scope);
        }
    }

    return result;
}

void ShaderGenerator::createAssignment(unsigned int output, unsigned int input) {
    validateAssignment(output, input);

    const ShaderValue& outputValue = getValue(output);

    if (!outputValue.availability.singleStage()) {
        throw std::runtime_error("ShaderGenerator: assignment target must belong to exactly one shader stage");
    }

    ValueOperation operation;
    operation.type = ValueOperationType::Assign;
    operation.output = output;
    operation.inputs = {input};
    operation.scope = outputValue.availability.singleScope();

    m_operations.push_back(std::move(operation));
}

ShaderExpr ShaderGenerator::createBinary(BinaryOperator op, unsigned int lhs, unsigned int rhs) {
    const ShaderValue& lhsValue = getValue(lhs);
    const ShaderValue& rhsValue = getValue(rhs);

    validateBinary(op, lhsValue, rhsValue);

    GlslType resultType = resolveBinaryResultType(op, lhsValue.type, rhsValue.type);
    ShaderAvailability availability = resolveExpressionAvailability({lhs, rhs});
    unsigned int output = createValue(resultType, ValueKind::Local, availability);

    m_values[output].initialProducerOperation = static_cast<int>(m_operations.size());

    ValueOperation operation;
    operation.type = ValueOperationType::Binary;
    operation.output = output;
    operation.inputs = {lhs, rhs};
    operation.binaryOperator = op;

    m_operations.push_back(std::move(operation));

    return ShaderExpr(this, output);
}

ShaderExpr ShaderGenerator::createUnary(UnaryOperator op, unsigned int input) {
    const ShaderValue& inputValue = getValue(input);

    validateUnary(op, inputValue);

    unsigned int output = createValue(inputValue.type, ValueKind::Local, inputValue.availability);

    m_values[output].initialProducerOperation = m_operations.size();

    ValueOperation operation;
    operation.type = ValueOperationType::Unary;
    operation.output = output;
    operation.inputs = {input};
    operation.unaryOperator = op;

    m_operations.push_back(std::move(operation));

    return ShaderExpr(this, output);
}

ShaderExpr ShaderGenerator::createSwizzle(unsigned int input, std::string swizzle) {
    const ShaderValue& inputValue = getValue(input);

    validateSwizzle(inputValue, swizzle);

    GlslType resultType = resolveSwizzleResultType(inputValue.type, swizzle.size());
    unsigned int output = createValue(resultType, ValueKind::Local, inputValue.availability);

    m_values[output].initialProducerOperation = m_operations.size();

    ValueOperation operation;
    operation.type = ValueOperationType::Swizzle;
    operation.output = output;
    operation.inputs = {input};
    operation.swizzle = std::move(swizzle);

    m_operations.push_back(std::move(operation));

    return ShaderExpr(this, output);
}

ShaderExpr ShaderGenerator::createConstant(GlslType type, std::string literal) {
    unsigned int id = createValue(type, ValueKind::Constant, {true, true});
    m_values[id].literal = std::move(literal);
    return ShaderExpr(this, id);
}

void ShaderGenerator::emitVertexInputs(
    std::ostringstream& vertex,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    const size_t stage = stageIndex(ValueScope::Vertex);

    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::VertexInput) continue;

        if (!reachable.values[id][stage]) continue;

        vertex << "layout(location = " << value.location << ") in " << glslTypeToString(value.type) << ' ' << value.name
               << ";\n";
        contract.interface.vertexInputs.push_back({value.type, value.location});
    }
}

void ShaderGenerator::emitVertexOutputs(
    std::ostringstream& vertex,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    const size_t stage = stageIndex(ValueScope::Vertex);

    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::VertexOutput) continue;

        if (!reachable.values[id][stage]) continue;

        vertex << "layout(location = " << value.location << ") out " << glslTypeToString(value.type) << ' '
               << value.name << ";\n";
        contract.interface.vertexOutputs.push_back({value.type, value.location});
        contract.interface.varyingNames.push_back(value.name);
    }
}

void ShaderGenerator::emitFragmentOutputs(
    std::ostringstream& fragment,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    const size_t stage = stageIndex(ValueScope::Fragment);

    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::FragmentOutput) continue;

        if (!reachable.values[id][stage]) continue;

        fragment << "layout(location = " << value.location << ") out " << glslTypeToString(value.type) << ' '
                 << value.name << ";\n";
        contract.interface.fragmentOutputs.push_back({value.type, value.location});
    }
}

void ShaderGenerator::emitUniforms(
    std::ostringstream& vertex,
    std::ostringstream& fragment,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::Uniform) {
            continue;
        }

        bool vertexLive = reachable.values[id][stageIndex(ValueScope::Vertex)];
        bool fragmentLive = reachable.values[id][stageIndex(ValueScope::Fragment)];

        if (vertexLive) {
            vertex << "uniform " << glslTypeToString(value.type) << " " << value.name << ";\n";
        }
        if (fragmentLive) {
            fragment << "uniform " << glslTypeToString(value.type) << " " << value.name << ";\n";
        }
        if (vertexLive || fragmentLive) {
            contract.interface.uniforms.insert(value.name);
        }
    }
}

void ShaderGenerator::emitVaryings(
    std::ostringstream& vertex,
    std::ostringstream& fragment,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    const size_t vertexStage = stageIndex(ValueScope::Vertex);
    const size_t fragmentStage = stageIndex(ValueScope::Fragment);

    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::Varying) continue;

        bool vertexLive = reachable.values[id][vertexStage];
        bool fragmentLive = reachable.values[id][fragmentStage];

        if (!vertexLive && !fragmentLive) continue;

        if (!vertexLive || !fragmentLive)
            throw std::runtime_error("Varying was not marked live on vertex and fragment stage");

        const char* qualifier = value.interpolation == VaryingInterpolation::Flat ? "flat " : "";

        vertex << qualifier << "out " << glslTypeToString(value.type) << ' ' << value.name << ";\n";
        fragment << qualifier << "in " << glslTypeToString(value.type) << ' ' << value.name << ";\n";

        contract.interface.varyingNames.push_back(value.name);
    }
}

void ShaderGenerator::emitLocals(std::ostringstream& out, ValueScope scope, const ShaderReachability& reachable) const {
    const size_t stage = stageIndex(scope);

    for (unsigned int id = 0; id < m_values.size(); id++) {
        const ShaderValue& value = m_values[id];

        if (value.kind != ValueKind::Local) continue;

        if (!reachable.values[id][stage]) continue;

        // If the initial producer is live, it will declare the variable.
        if (value.initialProducerOperation >= 0 && reachable.operations[value.initialProducerOperation][stage]) {
            continue;
        }

        out << GLSL_INDENT << glslTypeToString(value.type) << ' ' << value.name << ";\n";
    }
}

void ShaderGenerator::emitOperation(
    std::ostringstream& out,
    unsigned int operationId,
    const ValueOperation& operation
) const {
    out << GLSL_INDENT;

    const bool producesValue = operation.type != ValueOperationType::VoidCall;
    const bool inlineDeclaration = producesValue && m_values[operation.output].kind == ValueKind::Local &&
                                   m_values[operation.output].initialProducerOperation == static_cast<int>(operationId);
    if (inlineDeclaration) {
        out << glslTypeToString(getValue(operation.output).type) << " ";
    }
    switch (operation.type) {
        case ValueOperationType::Assign: emitAssign(out, operation); break;
        case ValueOperationType::Call: emitCall(out, operation); break;
        case ValueOperationType::VoidCall: emitVoidCall(out, operation); break;
        case ValueOperationType::Binary: emitBinary(out, operation); break;
        case ValueOperationType::Unary: emitUnary(out, operation); break;
        case ValueOperationType::Swizzle: emitSwizzle(out, operation); break;
        case ValueOperationType::Varying: emitVarying(out, operation); break;
    }
}

void ShaderGenerator::emitAssign(std::ostringstream& out, const ValueOperation& operation) const {
    out << getValueName(operation.output) << " = " << getValueName(operation.inputs[0]) << ";\n";
}

void ShaderGenerator::emitCall(std::ostringstream& out, const ValueOperation& operation) const {
    out << getValueName(operation.output) << " = " << operation.function << '(';
    for (size_t i = 0; i < operation.inputs.size(); i++) {
        if (i != 0) out << ", ";
        out << getValueName(operation.inputs[i]);
    }
    out << ");\n";
}

void ShaderGenerator::emitVoidCall(std::ostringstream& out, const ValueOperation& operation) const {
    out << operation.function << '(';
    for (size_t i = 0; i < operation.inputs.size(); i++) {
        if (i != 0) out << ", ";
        out << getValueName(operation.inputs[i]);
    }
    out << ");\n";
}

void ShaderGenerator::emitBinary(std::ostringstream& out, const ValueOperation& operation) const {
    const char* binaryOperatorString;
    switch (operation.binaryOperator) {
        case BinaryOperator::Add: binaryOperatorString = "+"; break;
        case BinaryOperator::Subtract: binaryOperatorString = "-"; break;
        case BinaryOperator::Multiply: binaryOperatorString = "*"; break;
        case BinaryOperator::Divide: binaryOperatorString = "/"; break;
        default: throw std::runtime_error("Unhandled binary operator in string conversion");
    }

    out << getValueName(operation.output) << " = " << getValueName(operation.inputs[0]) << ' ' << binaryOperatorString
        << ' ' << getValueName(operation.inputs[1]) << ";\n";
}

void ShaderGenerator::emitUnary(std::ostringstream& out, const ValueOperation& operation) const {
    const char* operatorString;
    switch (operation.unaryOperator) {
        case UnaryOperator::Negate: operatorString = "-"; break;
        default: throw std::runtime_error("Unhanled unary operator type in string conversion");
    }

    out << getValueName(operation.output) << " = " << operatorString << getValueName(operation.inputs[0]) << ";\n";
}

void ShaderGenerator::emitSwizzle(std::ostringstream& out, const ValueOperation& operation) const {
    out << getValueName(operation.output) << " = " << getValueName(operation.inputs[0]) << '.' << operation.swizzle
        << ";\n";
}

void ShaderGenerator::emitVarying(std::ostringstream& out, const ValueOperation& operation) const {
    out << getValueName(operation.output) << " = " << getValueName(operation.inputs[0]) << ";\n";
}

void ShaderGenerator::emitOperations(
    std::ostringstream& out,
    ValueScope scope,
    const ShaderReachability& reachable
) const {
    const size_t stage = stageIndex(scope);

    for (unsigned int id = 0; id < m_operations.size(); id++) {
        if (!reachable.operations[id][stage]) continue;
        emitOperation(out, id, m_operations[id]);
    }
}

void ShaderGenerator::collectModules(
    std::ostringstream& out,
    ValueScope scope,
    ShaderContract& contract,
    const ShaderReachability& reachable
) const {
    if (!m_modules) {
        lgr::lout.warn("Failed to collect modules because module registry was not set in shader generator");
        return;
    }

    const size_t stage = stageIndex(scope);

    std::unordered_set<const ShaderModule*> collected;

    for (unsigned int i = 0; i < m_operations.size(); i++) {
        if (!reachable.operations[i][stage]) continue;

        const ValueOperation& operation = m_operations[i];
        if (operation.type != ValueOperationType::Call && operation.type != ValueOperationType::VoidCall) {
            continue;
        }

        if (const ShaderModule* module = m_modules->findFunctionProvider(operation.function)) {
            collected.insert(module);
        }
    }

    for (const ShaderModule* module : collected) {
        out << "\n"
            << "// Module: " << module->modulePath() << "\n"
            << module->source() << "\n";

        for (const std::string& uniform : module->uniforms()) {
            contract.interface.uniforms.insert(uniform);
        }
    }
}

ShaderGenerator::ShaderGenerator(const ShaderModuleRegistry* modules) : m_modules(modules) {}

ShaderExpr ShaderGenerator::local(GlslType type, ValueScope scope) {
    return ShaderExpr(
        this, createValue(type, ValueKind::Local, {scope == ValueScope::Vertex, scope == ValueScope::Fragment})
    );
}

ShaderExpr ShaderGenerator::uniform(GlslType type, std::string name) {
    if (name.empty()) {
        throw std::runtime_error("ShaderGenerator: uniform name cannot be empty");
    }
    unsigned int id = createValue(type, ValueKind::Uniform, {true, true});
    m_values[id].name = std::move(name);
    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::vertexInput(unsigned int location, GlslType type) {
    validateLocation(location, ValueKind::VertexInput);

    unsigned int id = createValue(type, ValueKind::VertexInput, {true, false});
    m_values[id].name = "_vi" + std::to_string(id);
    m_values[id].location = location;
    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::vertexPosition() {
    unsigned int id = createValue(GlslType::Vec4, ValueKind::VertexPosition, {true, false});
    m_values[id].name = "gl_Position";
    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::vertexOutput(unsigned int location, GlslType type) {
    validateLocation(location, ValueKind::VertexOutput);

    unsigned int id = createValue(type, ValueKind::VertexOutput, {true, false});
    m_values[id].name = "_vo" + std::to_string(id);
    m_values[id].location = location;
    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::fragmentOutput(unsigned int location, GlslType type) {
    validateLocation(location, ValueKind::FragmentOutput);

    unsigned int id = createValue(type, ValueKind::FragmentOutput, {false, true});
    m_values[id].name = "_fo" + std::to_string(id);
    m_values[id].location = location;
    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::constant(bool value) { return createConstant(GlslType::Bool, value ? "true" : "false"); }

ShaderExpr ShaderGenerator::constant(int value) { return createConstant(GlslType::Int, std::to_string(value)); }

ShaderExpr ShaderGenerator::constant(unsigned int value) {
    return createConstant(GlslType::UInt, std::to_string(value) + "u");
}

ShaderExpr ShaderGenerator::constant(float value) { return createConstant(GlslType::Float, formatFloat(value)); }

ShaderExpr ShaderGenerator::constant(const glm::bvec2& value) {
    return createConstant(
        GlslType::BVec2,
        "bvec2(" + std::string(value.x ? "true" : "false") + ", " + std::string(value.y ? "true" : "false") + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::bvec3& value) {
    return createConstant(
        GlslType::BVec3,
        "bvec3(" + std::string(value.x ? "true" : "false") + ", " + std::string(value.y ? "true" : "false") + ", " +
            std::string(value.z ? "true" : "false") + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::bvec4& value) {
    return createConstant(
        GlslType::BVec4,
        "bvec4(" + std::string(value.x ? "true" : "false") + ", " + std::string(value.y ? "true" : "false") + ", " +
            std::string(value.z ? "true" : "false") + ", " + std::string(value.w ? "true" : "false") + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::ivec2& value) {
    return createConstant(GlslType::IVec2, "ivec2(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ")");
}

ShaderExpr ShaderGenerator::constant(const glm::ivec3& value) {
    return createConstant(
        GlslType::IVec3,
        "ivec3(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z) + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::ivec4& value) {
    return createConstant(
        GlslType::IVec4,
        "ivec4(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z) + ", " +
            std::to_string(value.w) + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::uvec2& value) {
    return createConstant(GlslType::UVec2, "uvec2(" + std::to_string(value.x) + "u, " + std::to_string(value.y) + "u)");
}

ShaderExpr ShaderGenerator::constant(const glm::uvec3& value) {
    return createConstant(
        GlslType::UVec3,
        "uvec3(" + std::to_string(value.x) + "u, " + std::to_string(value.y) + "u, " + std::to_string(value.z) + "u)"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::uvec4& value) {
    return createConstant(
        GlslType::UVec4,
        "uvec4(" + std::to_string(value.x) + "u, " + std::to_string(value.y) + "u, " + std::to_string(value.z) + "u, " +
            std::to_string(value.w) + "u)"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::vec2& value) {
    return createConstant(GlslType::Vec2, "vec2(" + formatFloat(value.x) + ", " + formatFloat(value.y) + ")");
}

ShaderExpr ShaderGenerator::constant(const glm::vec3& value) {
    return createConstant(
        GlslType::Vec3, "vec3(" + formatFloat(value.x) + ", " + formatFloat(value.y) + ", " + formatFloat(value.z) + ")"
    );
}

ShaderExpr ShaderGenerator::constant(const glm::vec4& value) {
    return createConstant(
        GlslType::Vec4,
        "vec4(" + formatFloat(value.x) + ", " + formatFloat(value.y) + ", " + formatFloat(value.z) + ", " +
            formatFloat(value.w) + ")"
    );
}

ShaderExpr ShaderGenerator::varying(ShaderExpr input, VaryingInterpolation interpolation) {
    const ShaderValue& inputValue = getValue(input.id());

    if (!inputValue.availability.vertex) {
        throw std::runtime_error("ShaderGenerator: varying input must be available in the vertex shader");
    }

    validateInput(inputValue);

    unsigned int id = createValue(inputValue.type, ValueKind::Varying, {false, true});

    m_values[id].initialProducerOperation = m_operations.size();

    m_values[id].name = "_i" + std::to_string(id);
    m_values[id].interpolation = interpolation;

    ValueOperation operation;
    operation.type = ValueOperationType::Varying;
    operation.output = id;
    operation.inputs = {input.id()};
    operation.scope = ValueScope::Vertex;

    m_operations.push_back(std::move(operation));

    return ShaderExpr(this, id);
}

ShaderExpr ShaderGenerator::call(GlslType returnType, std::string function, std::vector<ShaderExpr> inputs) {
    std::vector<unsigned int> inputIds;
    inputIds.reserve(inputs.size());

    for (const ShaderExpr& input : inputs) {
        inputIds.push_back(input.id());
    }

    const ShaderAvailability availability = resolveExpressionAvailability(inputIds);

    unsigned int output = createValue(returnType, ValueKind::Local, availability);

    m_values[output].initialProducerOperation = m_operations.size();

    validateCall(output, function, inputIds);

    ValueOperation operation;
    operation.type = ValueOperationType::Call;
    operation.output = output;
    operation.inputs = std::move(inputIds);
    operation.function = std::move(function);

    m_operations.push_back(std::move(operation));

    return ShaderExpr(this, output);
}

void ShaderGenerator::call(ValueScope scope, std::string function, std::vector<ShaderExpr> inputs) {
    std::vector<unsigned int> inputIds;
    inputIds.reserve(inputs.size());

    for (const ShaderExpr& input : inputs) {
        inputIds.push_back(input.id());
    }

    validateVoidCall(scope, function, inputIds);

    ValueOperation operation;
    operation.type = ValueOperationType::VoidCall;
    operation.inputs = std::move(inputIds);
    operation.function = std::move(function);
    operation.scope = scope;

    m_operations.push_back(std::move(operation));
}

ShaderContract ShaderGenerator::generate() const {
    const ShaderReachability reachable = analyzeReachability();

    std::ostringstream vertex;
    std::ostringstream fragment;

    vertex << GLSL_VERSION << "\n\n";
    fragment << GLSL_VERSION << "\n\n";

    // Declarations
    ShaderContract contract;
    emitVertexInputs(vertex, contract, reachable);
    emitVertexOutputs(vertex, contract, reachable);
    emitFragmentOutputs(fragment, contract, reachable);
    emitUniforms(vertex, fragment, contract, reachable);
    emitVaryings(vertex, fragment, contract, reachable);

    // Inject modules
    collectModules(vertex, ValueScope::Vertex, contract, reachable);
    collectModules(fragment, ValueScope::Fragment, contract, reachable);

    // Vertex main
    vertex << "\nvoid main() {\n";
    emitLocals(vertex, ValueScope::Vertex, reachable);
    emitOperations(vertex, ValueScope::Vertex, reachable);
    vertex << "}\n";

    // Fragment main
    fragment << "\nvoid main() {\n";
    emitLocals(fragment, ValueScope::Fragment, reachable);
    emitOperations(fragment, ValueScope::Fragment, reachable);
    fragment << "}\n";

    contract.cpuShader.vertexShader = vertex.str();
    contract.cpuShader.fragmentShader = fragment.str();

    return contract;
}