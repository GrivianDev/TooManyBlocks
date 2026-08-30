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
    FXAARenderpass();
    virtual ~FXAARenderpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);
};

#endif
