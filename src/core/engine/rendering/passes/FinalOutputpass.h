#ifndef TOOMANYBLOCKS_FINALOUTPUTPASS_H
#define TOOMANYBLOCKS_FINALOUTPUTPASS_H

#include "engine/rendering/opengl/Shader.h"
#include "engine/rendering/passes/Renderpass.h"

class FinalOutputpass : public Renderpass {
private:
    Shader m_outputShader;

protected:
    virtual void prepare(
        RenderContext& context,
        RenderResources& resources,
        const ApplicationContext& appContext
    ) override;
    virtual void execute(
        RenderContext& context,
        RenderResources& resources,
        const ApplicationContext& appContext
    ) override;
    virtual void cleanup(
        RenderContext& context,
        RenderResources& resources,
        const ApplicationContext& appContext
    ) override;

public:
    FinalOutputpass();
    virtual ~FinalOutputpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;
};

#endif
