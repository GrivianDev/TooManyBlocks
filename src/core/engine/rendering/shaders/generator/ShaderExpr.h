#ifndef TOOMANYBLOCKS_SHADEREXPR_H
#define TOOMANYBLOCKS_SHADEREXPR_H

#include "engine/rendering/shaders/generator/Glsl.h"

class ShaderGenerator;

class ShaderExpr {
    friend class ShaderGenerator;

private:
    ShaderGenerator* m_generator;
    unsigned int m_id;

    inline ShaderExpr(ShaderGenerator* generator, unsigned int id) : m_generator(generator), m_id(id) {}

public:
    inline unsigned int id() const { return m_id; }

    GlslType type() const;
    ShaderAvailability availability() const;

    ShaderExpr x() const;
    ShaderExpr y() const;
    ShaderExpr z() const;
    ShaderExpr w() const;

    ShaderExpr r() const;
    ShaderExpr g() const;
    ShaderExpr b() const;
    ShaderExpr a() const;

    ShaderExpr xyz() const;
    ShaderExpr xy() const;
    ShaderExpr rgb() const;
    ShaderExpr rgba() const;

    void assign(const ShaderExpr& expr);

    ShaderExpr operator+(const ShaderExpr& rhs) const;
    ShaderExpr operator-(const ShaderExpr& rhs) const;
    ShaderExpr operator*(const ShaderExpr& rhs) const;
    ShaderExpr operator/(const ShaderExpr& rhs) const;

    ShaderExpr operator-() const;
};

#endif
