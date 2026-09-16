#ifndef TOOMANYBLOCKS_TRANSPARENCYRENDERPASS_H
#define TOOMANYBLOCKS_TRANSPARENCYRENDERPASS_H

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/passes/Renderpass.h"

class TransparencyRenderpass : public Renderpass {
private:
    FrameBuffer m_accAndResBuffer;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    bool accepts(const Renderable* obj) const override;
    ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override;

public:
    TransparencyRenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~TransparencyRenderpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);
};

#endif
