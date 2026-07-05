#ifndef TOOMANYBLOCKS_FXAARENDERPASS_H
#define TOOMANYBLOCKS_FXAARENDERPASS_H

#include "engine/rendering/lowlevelapi/Shader.h"
#include "engine/rendering/renderpasses/Renderpass.h"
#include "engine/rendering/lowlevelapi/FrameBuffer.h"

class FXAARenderpass : public Renderpass {
private:
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
};

#endif
