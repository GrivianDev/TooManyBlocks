#include "Renderpass.h"

#include <chrono>

void Renderpass::drawRenderable(Renderable* obj) { obj->draw(); }

void Renderpass::addForPass(const std::vector<Renderable*>& objects) {
    for (Renderable* object : objects) {
        if (accepts(object)) m_passObjects.insert(object);
    }
}

void Renderpass::batchForPass(const RenderContext& context) {
    for (auto& [shaderKey, renderables] : m_shaderBatches) {
        renderables.clear();
    }
    for (Renderable* renderable : m_passObjects) {
        ShaderKey key = makeShaderKey(renderable, context);
        m_shaderBatches[key].push_back(renderable);
    }
}

void Renderpass::renderBatches(const RenderContext& context) {
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

            m_objectsRendered++;
        }
    }
}

void Renderpass::run(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    auto start = std::chrono::high_resolution_clock::now();
    m_objectsRendered = 0;
    prepare(context, resources, appContext);
    execute(context, resources, appContext);
    cleanup(context, resources, appContext);
    auto end = std::chrono::high_resolution_clock::now();
    m_lastRunTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}
