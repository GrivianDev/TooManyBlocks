#ifndef TOOMANYBLOCKS_MAINRENDERPASS_H
#define TOOMANYBLOCKS_MAINRENDERPASS_H

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/passes/Renderpass.h"

class OpaqueRenderpass : public Renderpass {
private:
    FrameBuffer m_opaqueBuffer;
    bool m_debugPolygonModeEnabled;
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
    OpaqueRenderpass();
    virtual ~OpaqueRenderpass() = default;

    virtual const char* name() override;

    virtual void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);

    inline bool isDebugPolygonModeEnabled() const { return m_debugPolygonModeEnabled; }

    inline void setDebugPolygonModeEnabled(bool enabled) { m_debugPolygonModeEnabled = enabled; }
};

#endif
