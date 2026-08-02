#ifndef TOOMANYBLOCKS_SHADOWPASS_H
#define TOOMANYBLOCKS_SHADOWPASS_H

#include <stddef.h>

#include "engine/rendering/renderpasses/Renderpass.h"
#include "engine/rendering/renderpasses/processors/LightProcessor.h"

class ShadowRenderpass : public Renderpass {
private:
    LightProcessor m_lightProcessor;
    size_t m_objectsProcessed;
    size_t m_processedLightCount;
    size_t m_shadowMapCount;

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
    ShadowRenderpass();
    virtual ~ShadowRenderpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;

    inline LightProcessor& getLightProcessor() { return m_lightProcessor; };
};

#endif
