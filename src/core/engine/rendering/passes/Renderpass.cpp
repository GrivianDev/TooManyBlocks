#include "Renderpass.h"

#include <chrono>

void Renderpass::batchByMaterialForPass(const std::vector<Renderable*>& meshBuff, PassType type) {
    for (Renderable* mesh : meshBuff) {
        if (mesh->getMaterial()->supportsPass(type)) {
            m_materialBatches[mesh->getMaterial().get()].push_back(mesh);
        }
    }
}

void Renderpass::run(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    auto start = std::chrono::high_resolution_clock::now();
    prepare(context, resources, appContext);
    execute(context, resources, appContext);
    cleanup(context, resources, appContext);
    auto end = std::chrono::high_resolution_clock::now();
    m_lastRunTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}
