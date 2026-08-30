#ifndef TOOMANYBLOCKS_TRANSFORMFEEBACKPASS_H
#define TOOMANYBLOCKS_TRANSFORMFEEBACKPASS_H

#include <stddef.h>

#include "engine/rendering/passes/Renderpass.h"

class TransformFeedbackpass : public Renderpass {
private:
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
    virtual ~TransformFeedbackpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;
};

#endif
