#include "ShaderExpr.h"

#include "engine/rendering/shaders/generator/ShaderGenerator.h"

GlslType ShaderExpr::type() const { return m_generator->getValue(m_id).type; }

ShaderAvailability ShaderExpr::availability() const { return m_generator->getValue(m_id).availability; }

ShaderExpr ShaderExpr::x() const { return m_generator->createSwizzle(m_id, "x"); }

ShaderExpr ShaderExpr::y() const { return m_generator->createSwizzle(m_id, "y"); }

ShaderExpr ShaderExpr::z() const { return m_generator->createSwizzle(m_id, "z"); }

ShaderExpr ShaderExpr::w() const { return m_generator->createSwizzle(m_id, "w"); }

ShaderExpr ShaderExpr::r() const { return m_generator->createSwizzle(m_id, "r"); }

ShaderExpr ShaderExpr::g() const { return m_generator->createSwizzle(m_id, "g"); }

ShaderExpr ShaderExpr::b() const { return m_generator->createSwizzle(m_id, "b"); }

ShaderExpr ShaderExpr::a() const { return m_generator->createSwizzle(m_id, "a"); }

ShaderExpr ShaderExpr::xyz() const { return m_generator->createSwizzle(m_id, "xyz"); }

ShaderExpr ShaderExpr::xy() const { return m_generator->createSwizzle(m_id, "xy"); }

ShaderExpr ShaderExpr::rgb() const { return m_generator->createSwizzle(m_id, "rgb"); }

ShaderExpr ShaderExpr::rgba() const { return m_generator->createSwizzle(m_id, "rgba"); }

void ShaderExpr::assign(const ShaderExpr& expr) { m_generator->createAssignment(m_id, expr.id()); }

ShaderExpr ShaderExpr::operator+(const ShaderExpr& rhs) const {
    return m_generator->createBinary(ShaderGenerator::BinaryOperator::Add, m_id, rhs.m_id);
}

ShaderExpr ShaderExpr::operator-(const ShaderExpr& rhs) const {
    return m_generator->createBinary(ShaderGenerator::BinaryOperator::Subtract, m_id, rhs.m_id);
}

ShaderExpr ShaderExpr::operator*(const ShaderExpr& rhs) const {
    return m_generator->createBinary(ShaderGenerator::BinaryOperator::Multiply, m_id, rhs.m_id);
}

ShaderExpr ShaderExpr::operator/(const ShaderExpr& rhs) const {
    return m_generator->createBinary(ShaderGenerator::BinaryOperator::Divide, m_id, rhs.m_id);
}

ShaderExpr ShaderExpr::operator-() const {
    return m_generator->createUnary(ShaderGenerator::UnaryOperator::Negate, m_id);
}
