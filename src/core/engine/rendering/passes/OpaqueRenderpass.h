#ifndef TOOMANYBLOCKS_MAINRENDERPASS_H
#define TOOMANYBLOCKS_MAINRENDERPASS_H

#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/passes/Renderpass.h"

class OpaqueRenderpass : public Renderpass {
private:
    FrameBuffer m_opaqueBuffer;
    bool m_debugPolygonModeEnabled;

protected:
    void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;
    void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) override;

    bool accepts(const Renderable* obj) const override;
    ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const override;

public:
    OpaqueRenderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder);
    ~OpaqueRenderpass() = default;

    const char* name() override;

    void putDebugInfo(DebugReport& report) override;

    void createBuffers(RenderContext& context);

    inline bool isDebugPolygonModeEnabled() const { return m_debugPolygonModeEnabled; }

    inline void setDebugPolygonModeEnabled(bool enabled) { m_debugPolygonModeEnabled = enabled; }
};

#endif
