#ifndef TOOMANYBLOCKS_SSAORENDERPASS_H
#define TOOMANYBLOCKS_SSAORENDERPASS_H

#include <stddef.h>

#include "engine/rendering/passes/Renderpass.h"
#include "engine/rendering/passes/processors/SSAOProcessor.h"

class SSAORenderpass : public Renderpass {
private:
    SSAOProcessor m_ssaoProcessor;
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
    virtual ~SSAORenderpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;

    inline SSAOProcessor& getSSAOProcessor() { return m_ssaoProcessor; }
};

#endif
