#ifndef TOOMANYBLOCKS_TRANSPARENCYRENDERPASS_H
#define TOOMANYBLOCKS_TRANSPARENCYRENDERPASS_H

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/passes/Renderpass.h"

class TransparencyRenderpass : public Renderpass {
private:
    FrameBuffer m_accAndResBuffer;
    size_t m_objectsProcessed;

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
    TransparencyRenderpass();
    virtual ~TransparencyRenderpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);
};

#endif
