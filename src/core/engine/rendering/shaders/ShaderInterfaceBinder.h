#ifndef TOOMANYBLOCKS_SHADERINTERFACEBINDER_H
#define TOOMANYBLOCKS_SHADERINTERFACEBINDER_H

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "engine/rendering/shaders/PassTypes.h"
#include "engine/rendering/shaders/RenderProgram.h"
#include "engine/scene/renderables/Renderable.h"

struct RenderContext;
struct RenderResources;

class ShaderInterfaceBinder {
private:
    enum class BindingStage {
        Pass,
        Material,
        Renderable
    };

    struct BindingContext {
        PassType pass;

        const RenderContext& renderContext;

        const Material* material;
        const Renderable* renderable;
    };

    struct Binding {
        BindingStage stage;
        std::function<void(Shader&, const BindingContext&)> function;
    };

    std::unordered_map<std::string, Binding> m_bindings;
    std::unordered_set<std::string> m_boundUniforms;

    void addBinding(std::string name, BindingStage stage, std::function<void(Shader&, const BindingContext&)> function);

    void bindStage(BindingStage stage, RenderProgram& program, const BindingContext& context);

public:
    ShaderInterfaceBinder();

    void bindPass(PassType pass, RenderProgram& program, const RenderContext& context);
    void bindMaterial(PassType pass, RenderProgram& program, const RenderContext& context, const Material& material);
    void bindRenderable(PassType pass, RenderProgram& program, const RenderContext& context, const Renderable* obj);
};

#endif
