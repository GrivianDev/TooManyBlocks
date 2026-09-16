#ifndef TOOMANYBLOCKS_SSAORENDERPASS_H
#define TOOMANYBLOCKS_SSAORENDERPASS_H

#include <stddef.h>

#include "engine/rendering/passes/Renderpass.h"
#include "engine/rendering/passes/processors/SSAOProcessor.h"

class SSAORenderpass : public Renderpass {
private:
    SSAOProcessor m_ssaoProcessor;

    bool accepts(const Renderable* obj) const override;
    ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

public:
    SSAORenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~SSAORenderpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;

    inline SSAOProcessor& getSSAOProcessor() { return m_ssaoProcessor; }
};

#endif
