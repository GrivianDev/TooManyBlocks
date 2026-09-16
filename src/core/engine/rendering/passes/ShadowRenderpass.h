#ifndef TOOMANYBLOCKS_SHADOWPASS_H
#define TOOMANYBLOCKS_SHADOWPASS_H

#include <stddef.h>

#include "engine/rendering/passes/Renderpass.h"
#include "engine/rendering/passes/processors/LightProcessor.h"

class ShadowRenderpass : public Renderpass {
private:
    LightProcessor m_lightProcessor;
    size_t m_processedLightCount;
    size_t m_shadowMapCount;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    bool accepts(const Renderable* obj) const override;
    ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override;

public:
    ShadowRenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~ShadowRenderpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;

    inline LightProcessor& getLightProcessor() { return m_lightProcessor; };
};

#endif
