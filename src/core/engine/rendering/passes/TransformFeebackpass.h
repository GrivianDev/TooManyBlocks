#ifndef TOOMANYBLOCKS_TRANSFORMFEEBACKPASS_H
#define TOOMANYBLOCKS_TRANSFORMFEEBACKPASS_H

#include <stddef.h>

#include "engine/rendering/passes/Renderpass.h"

class TransformFeedbackpass : public Renderpass {
protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    bool accepts(const Renderable* obj) const override;
    ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override;

    void drawRenderable(Renderable* obj) override;

public:
    TransformFeedbackpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    virtual ~TransformFeedbackpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;
};

#endif
