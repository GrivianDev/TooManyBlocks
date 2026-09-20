#ifndef TOOMANYBLOCKS_RENDERPASS_H
#define TOOMANYBLOCKS_RENDERPASS_H

#include <unordered_map>
#include <vector>

#include "engine/rendering/passes/debug/DebugReport.h"
#include "engine/rendering/shaders/ShaderInterfaceBinder.h"
#include "engine/rendering/shaders/ShaderKey.h"
#include "engine/rendering/shaders/ShaderManager.h"
#include "engine/scene/renderables/Renderable.h"

struct RenderContext;
struct RenderResources;
struct ApplicationContext;

class Renderpass {
private:
    bool m_isEnabled;

protected:
    ShaderManager* m_shaderManager;
    ShaderInterfaceBinder* m_binder;
    size_t m_objectsProcessed;

    std::unordered_map<ShaderKey, std::vector<Renderable*>, ShaderKeyHash> m_shaderBatches;
    float m_lastRunTimeMs;

    virtual void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {};
    virtual void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) = 0;
    virtual void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {};

    virtual bool accepts(const Renderable* obj) const = 0;
    virtual ShaderKey makeShaderKey(const Renderable* obj, const RenderContext& context) const = 0;

    virtual void drawRenderable(Renderable* obj);

    void filterForPass(const std::vector<Renderable*>& input, std::vector<Renderable*>& output);

    void batchForPass(const std::vector<Renderable*>& renderables, const RenderContext& context);

    void renderBatches(RenderContext& context, RenderResources& resources);

public:
    Renderpass(ShaderManager* shaderManager, ShaderInterfaceBinder* binder)
        : m_isEnabled(true), m_shaderManager(shaderManager), m_binder(binder), m_objectsProcessed(0) {}
    virtual ~Renderpass() = default;

    virtual const char* name() = 0;

    void run(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext);

    virtual void putDebugInfo(DebugReport& report) = 0;

    inline void setEnabled(bool enabled) { m_isEnabled = enabled; }

    inline bool isEnabled() const { return m_isEnabled; }
};

#endif
