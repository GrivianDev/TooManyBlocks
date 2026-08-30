#include "FXAARenderpass.h"

#include <GL/glew.h>

#include "Application.h"
#include "engine/assets/AssetPaths.h"
#include "engine/assets/loaders/ShaderLoader.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"

void FXAARenderpass::prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    if (context.screenResChanged || m_fxaaBuffer.getAttachedTextures().empty()) {
        createBuffers(context);
    }

    m_fxaaBuffer.bind();
    GLCALL(glDisable(GL_DEPTH_TEST));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    glm::uvec2 screenRes = context.currScreenRes;
    GLCALL(glViewport(0, 0, screenRes.x, screenRes.y));
}

void FXAARenderpass::execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    m_fxaaShader.use();
    context.resolverInfo.output->bindToUnit(0);
    m_fxaaShader.setUniform("u_inputTexture", 0);
    m_fxaaShader.setUniform("u_texelSize", 1.0f / glm::vec2(context.currScreenRes.x, context.currScreenRes.y));

    appContext.renderer->drawFullscreenQuad();
}

void FXAARenderpass::cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    GLCALL(glEnable(GL_DEPTH_TEST));
    context.fxaaInfo.output = m_fxaaBuffer.getAttachedTextures().at(0).get();
}

FXAARenderpass::FXAARenderpass() {
    CPUShader cpuShader = loadShaderFromFile(Res::Shader::FXAA, ShaderLoadOption::VertexAndFragment);
    m_fxaaShader = Shader::create(cpuShader.vertexShader, cpuShader.fragmentShader);
    m_fxaaBuffer = FrameBuffer::create();
}

const char* FXAARenderpass::name() { return "FXAA Renderpass"; }

void FXAARenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.endGroup();
}

void FXAARenderpass::createBuffers(RenderContext& context) {
    m_fxaaBuffer.clearAttachedTextures();
    m_fxaaBuffer.attachTexture(
        std::make_shared<Texture>(
            Texture::create(TextureType::Color, context.currScreenRes.x, context.currScreenRes.y, 3)
        )
    );
}
