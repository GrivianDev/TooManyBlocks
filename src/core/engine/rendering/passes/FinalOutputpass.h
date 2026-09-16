#ifndef TOOMANYBLOCKS_FINALOUTPUTPASS_H
#define TOOMANYBLOCKS_FINALOUTPUTPASS_H

#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/passes/Renderpass.h"

class FinalOutputpass : public Renderpass {
private:
    Shader m_outputShader;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    inline bool accepts(const Renderable* obj) const override { return true; }
    inline ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override { return {}; };

public:
    FinalOutputpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~FinalOutputpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;
};

#endif
