#include "FXAARenderpass.h"

#include <GL/glew.h>

#include "AppConstants.h"
#include "Application.h"
#include "engine/rendering/Frustum.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/lowlevelapi/FrameBuffer.h"
#include "engine/resource/loaders/ShaderLoader.h"

void FXAARenderpass::prepare(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    FrameBuffer::bindDefault();
    GLCALL(glDisable(GL_DEPTH_TEST));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    glm::uvec2 screenRes = context.currScreenRes;
    GLCALL(glViewport(0, 0, screenRes.x, screenRes.y));
}

void FXAARenderpass::execute(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    m_fxaaShader.use();
    context.resolverInfo.output->bindToUnit(0);
    m_fxaaShader.setUniform("u_inputTexture", 0);

    m_fxaaShader.setUniform("u_texelSize", glm::vec2(1.0f / context.currScreenRes.x, 1.0f / context.currScreenRes.y));

    appContext.renderer->drawFullscreenQuad();
}

void FXAARenderpass::cleanup(RenderContext& context, RenderResources& resources, const ApplicationContext& appContext) {
    GLCALL(glEnable(GL_DEPTH_TEST));
}

FXAARenderpass::FXAARenderpass() {
    ApplicationContext* context = Application::getContext();
    CPUShader cpuShader = loadShaderFromFile(Res::Shader::FXAA, ShaderLoadOption::VertexAndFragment);

    m_fxaaShader = Shader::create(cpuShader.vertexShader, cpuShader.fragmentShader);
}

const char* FXAARenderpass::name() { return "FXAA Renderpass"; }

void FXAARenderpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.endGroup();
}
