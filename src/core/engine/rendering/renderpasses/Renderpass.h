#ifndef TOOMANYBLOCKS_RENDERPASS_H
#define TOOMANYBLOCKS_RENDERPASS_H

#include <unordered_map>
#include <vector>

#include "engine/rendering/Renderable.h"
#include "engine/rendering/mat/Material.h"
#include "engine/rendering/renderpasses/DebugReport.h"

struct RenderContext;
struct RenderResources;
struct ApplicationContext;

class Renderpass {
private:
    bool m_isEnabled;

protected:
    std::unordered_map<Material*, std::vector<Renderable*>> m_materialBatches;
    float m_lastRunTimeMs;

    virtual void prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {};
    virtual void execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) = 0;
    virtual void cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {};

    void batchByMaterialForPass(const std::vector<Renderable*>& meshBuff, PassType type);

public:
    Renderpass() : m_isEnabled(true) {}
    virtual ~Renderpass() = default;

    virtual const char* name() = 0;

    void run(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext);

    virtual void putDebugInfo(DebugReport& report) = 0;

    inline void setEnabled(bool enabled) { m_isEnabled = enabled; }

    inline bool isEnabled() const { return m_isEnabled; }
};

#endif
