#include "FinalOutputpass.h"

#include <GL/glew.h>

#include "AppConstants.h"
#include "Application.h"
#include "engine/assets/loaders/ShaderLoader.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/lowlevelapi/FrameBuffer.h"

void FinalOutputpass::prepare(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    FrameBuffer::bindDefault();
    GLCALL(glDisable(GL_DEPTH_TEST));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));
    glm::uvec2 screenRes = context.currScreenRes;
    GLCALL(glViewport(0, 0, screenRes.x, screenRes.y));
}

void FinalOutputpass::execute(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    const Texture* output = context.resolverInfo.output;
    if (context.fxaaInfo.enabled) output = context.fxaaInfo.output;

    m_outputShader.use();
    output->bindToUnit(0);
    m_outputShader.setUniform("u_inputTexture", 0);

    appContext.renderer->drawFullscreenQuad();
}

void FinalOutputpass::cleanup(
    RenderContext& context,
    RenderResources& resources,
    const ApplicationContext& appContext
) {
    GLCALL(glEnable(GL_DEPTH_TEST));
}

FinalOutputpass::FinalOutputpass() {
    CPUShader cpuShader = loadShaderFromFile(Res::Shader::FXAA, ShaderLoadOption::VertexAndFragment);
    m_outputShader = Shader::create(cpuShader.vertexShader, cpuShader.fragmentShader);
}

const char* FinalOutputpass::name() { return "Final Outputpass"; }

void FinalOutputpass::putDebugInfo(DebugReport& report) {
    report.beginGroup(name());
    report.addTimeMs("Processing Time", m_lastRunTimeMs);
    report.endGroup();
}
