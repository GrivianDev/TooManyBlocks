#ifndef TOOMANYBLOCKS_RENDERPROGRAM_H
#define TOOMANYBLOCKS_RENDERPROGRAM_H

#include <memory>

#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/shaders/ShaderInterface.h"

class RenderProgram {
private:
    std::unique_ptr<Shader> m_shader;
    ShaderInterface m_interface;

public:
    RenderProgram(std::unique_ptr<Shader> shader, ShaderInterface interface)
        : m_shader(std::move(shader)), m_interface(std::move(interface)) {}

    Shader& shader() { return *m_shader; }
    const Shader& shader() const { return *m_shader; }

    const ShaderInterface& interface() const { return m_interface; }
};

#endif
