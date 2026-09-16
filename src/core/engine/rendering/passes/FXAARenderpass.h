#ifndef TOOMANYBLOCKS_FXAARENDERPASS_H
#define TOOMANYBLOCKS_FXAARENDERPASS_H

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/passes/Renderpass.h"

class FXAARenderpass : public Renderpass {
private:
    FrameBuffer m_fxaaBuffer;
    Shader m_fxaaShader;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    inline bool accepts(const Renderable* obj) const override { return true; }
    inline ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override { return {}; };

public:
    FXAARenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~FXAARenderpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);
};

#endif
