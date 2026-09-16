#include "Renderpass.h"

#include <chrono>

void Renderpass::drawRenderable(Renderable* obj) { obj->draw(); }

void Renderpass::filterForPass(const std::vector<Renderable*>& input, std::vector<Renderable*>& output) {
    output.clear();
    for (Renderable* renderable : input) {
        if (accepts(renderable)) output.push_back(renderable);
    }
}

void Renderpass::batchForPass(const std::vector<Renderable*>& renderables, const RenderContext& context) {
    m_shaderBatches.clear();
    for (Renderable* renderable : renderables) {
        ShaderKey key = makeShaderKey(renderable, context);
        m_shaderBatches[key].push_back(renderable);
    }
}

void Renderpass::renderBatches(RenderContext& context, RenderResources& resources) {
    for (const auto& [shaderKey, renderables] : m_shaderBatches) {
        RenderProgram& program = m_shaderManager->loadProgram(shaderKey);
        m_binder->bindPass(shaderKey.pass, program, context);

        Material* currentMaterial = nullptr;
        for (Renderable* renderable : renderables) {
            Material* material = renderable->getMaterial().get();
            if (material != currentMaterial) {
                m_binder->bindMaterial(shaderKey.pass, program, context, *material);
                currentMaterial = material;
            }

            m_binder->bindRenderable(shaderKey.pass, program, context, renderable);
            drawRenderable(renderable);

            m_objectsProcessed++;
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
