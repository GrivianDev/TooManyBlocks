#include "ShaderInterfaceBinder.h"

#include "Logger.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/particles/ParticleSystem.h"
#include "engine/scene/renderables/SkeletalMesh.h"

void ShaderInterfaceBinder::addBinding(
    std::string name,
    BindingStage stage,
    std::function<void(Shader&, const BindingContext&)> function
) {
    m_bindings.emplace(std::move(name), Binding{stage, std::move(function)});
}

void ShaderInterfaceBinder::bindStage(BindingStage stage, RenderProgram& program, const BindingContext& context) {
    for (const std::string& name : program.interface().uniforms) {
        auto it = m_bindings.find(name);
        if (it == m_bindings.end()) {
            // No binding registered for this uniform.
            continue;
        }

        const Binding& binding = it->second;
        if (binding.stage != stage) {
            continue;
        }

        binding.function(program.shader(), context);
        m_boundUniforms.insert(name);
    }
}

ShaderInterfaceBinder::ShaderInterfaceBinder() {
    // Pass
    addBinding("u_view", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_view", ctx.renderContext.viewport.view);
    });
    addBinding("u_viewProjection", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_viewProjection", ctx.renderContext.viewport.viewProjection);
    });
    addBinding("u_lightViewProjection", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_lightViewProjection", ctx.renderContext.lighting.viewport.viewProjection);
    });
    addBinding("u_cameraPosition", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_cameraPosition", ctx.renderContext.viewport.transform.getPosition());
    });
    addBinding("u_projection", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_projection", ctx.renderContext.viewport.projection);
    });
    addBinding("u_cameraRight", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_cameraRight", ctx.renderContext.viewport.transform.getRight());
    });
    addBinding("u_cameraUp", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_cameraUp", ctx.renderContext.viewport.transform.getUp());
    });
    addBinding("u_screenResolution", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_screenResolution", ctx.renderContext.currScreenRes);
    });
    addBinding("u_lightCount", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_lightCount", static_cast<int>(ctx.renderContext.lighting.activeLightsCount));
    });
    addBinding("LightsBlock", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.bindUniformBuffer("LightsBlock", *ctx.renderContext.lighting.lightBuff);
    });
    addBinding("ShadowMapsBlock", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.bindUniformBuffer("ShadowMapsBlock", *ctx.renderContext.lighting.shadowMapBuff);
    });
    addBinding("u_shadowMapAtlas", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        const int offset = 2;
        for (int atlasIndex = 0; atlasIndex < SHADOW_ATLAS_COUNT; atlasIndex++) {
            if (const Texture* shadowAtlas = ctx.renderContext.lighting.shadowMapAtlases[atlasIndex]) {
                const std::string idxStr = std::to_string(atlasIndex);
                shadowAtlas->bindToUnit(atlasIndex + offset);
                shader.setUniform("u_shadowMapAtlas[" + idxStr + "]", atlasIndex + offset);
            }
        }
    });

    addBinding("u_ssaoTexture", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        ctx.renderContext.ssao.output->bindToUnit(1);
        shader.setUniform("u_ssaoTexture", 1);
    });

    // Material
    addBinding("u_color", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_color", ctx.material->baseColor);
    });
    addBinding("u_specularFactor", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_specularFactor", ctx.material->specularFactor);
    });
    addBinding("u_specularExponent", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_specularExponent", ctx.material->specularExponent);
    });
    addBinding("u_alphaCutoff", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_alphaCutoff", ctx.material->alphaCutoff);
    });
    addBinding("u_texture", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        ctx.material->baseColorTexture.value().bindToUnit(0);
        shader.setUniform("u_texture", 0);
    });
    addBinding("u_textureAtlas", BindingStage::Material, [](Shader& shader, const BindingContext& ctx) {
        ctx.material->baseColorTexture.value().bindToUnit(0);
        shader.setUniform("u_textureAtlas", 0);
    });
    addBinding("u_textureSize", BindingStage::Material, [](Shader& shader, const BindingContext&) {
        shader.setUniform("u_textureSize", 16u);
    });

    // Renderable
    addBinding("u_mvp", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform(
            "u_mvp",
            ctx.renderContext.viewport.viewProjection * ctx.renderable->getRenderableTransform().getModelMatrix()
        );
    });
    addBinding("u_model", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_model", ctx.renderable->getRenderableTransform().getModelMatrix());
    });
    addBinding("u_position", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_position", ctx.renderable->getRenderableTransform().getPosition());
    });
    addBinding("JointMatrices", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* s = dynamic_cast<const SkeletalMesh*>(ctx.renderable)) {
            shader.bindUniformBuffer("JointMatrices", *s->getJointMatrices());
        }
    });

    // Particle
    addBinding("u_deltaTime", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_deltaTime", ctx.renderContext.deltaTime);
    });
    addBinding("u_time", BindingStage::Pass, [](Shader& shader, const BindingContext& ctx) {
        shader.setUniform("u_time", ctx.renderContext.elapsedTime);
    });
    addBinding("ParticleModulesBlock", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.bindUniformBuffer("ParticleModulesBlock", *p->getModulesUBO());
        }
    });
    addBinding("u_moduleCount", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.setUniform("u_moduleCount", p->getModulesCount());
        }
    });
    addBinding("u_spawnCount", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.setUniform("u_spawnCount", p->getSpawnCount());
        }
    });
    addBinding("u_particleSpawnOffset", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.setUniform("u_particleSpawnOffset", p->getParticleSpawnOffset());
        }
    });
    addBinding("u_allocatedParticleCount", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.setUniform("u_allocatedParticleCount", p->getAllocatedParticleCount());
        }
    });
    addBinding("u_flags", BindingStage::Renderable, [](Shader& shader, const BindingContext& ctx) {
        if (const auto* p = dynamic_cast<const ParticleSystem*>(ctx.renderable)) {
            shader.setUniform("u_flags", p->getFlags());
        }
    });
}

void ShaderInterfaceBinder::bindPass(PassType pass, RenderProgram& program, const RenderContext& context) {
    BindingContext bindingContext{pass, context, nullptr, nullptr};
    bindStage(BindingStage::Pass, program, bindingContext);
}

void ShaderInterfaceBinder::bindMaterial(
    PassType pass,
    RenderProgram& program,
    const RenderContext& context,
    const Material& material
) {
    BindingContext bindingContext{pass, context, &material, nullptr};
    bindStage(BindingStage::Material, program, bindingContext);
}

void ShaderInterfaceBinder::bindRenderable(
    PassType pass,
    RenderProgram& program,
    const RenderContext& context,
    const Renderable* obj
) {
    BindingContext bindingContext{pass, context, nullptr, obj};
    bindStage(BindingStage::Renderable, program, bindingContext);

    for (const std::string& uniform : program.interface().uniforms) {
        if (m_boundUniforms.find(uniform) == m_boundUniforms.end()) {
            lgr::lout.warn(
                "Uniform \"" + uniform + "\" declared by interface of shader with id " +
                std::to_string(program.shader().rendererId()) + " was not bound"
            );
        }
    }
}
