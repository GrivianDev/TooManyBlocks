#include "SettingsMenu.h"

#include <imgui.h>

#include "Application.h"
#include "engine/GameInstance.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/renderpasses/FXAARenderpass.h"
#include "engine/rendering/renderpasses/SSAORenderpass.h"
#include "platform/window/WindowManager.h"

const char* qualityNames[] = {"Low", "Medium", "High"};

const char* windowModes[] = {"Windowed", "Borderless", "Fullscreen"};

static void SettingsSection(const char* name) {
    ImGui::Spacing();
    ImGui::Spacing();

    ScopedFont font(UI::manager().getFont(35));
    ImGui::Text("%s", name);

    ImGui::Separator();

    ImGui::Spacing();
}

static void InfoRow(const char* label, const std::string& value) {
    ImGui::Text("%s", label);
    ImGui::SameLine(300.0f);
    ImGui::Text("%s", value.c_str());
}

static void InfoRow(const char* label, const char* value) {
    ImGui::Text("%s", label);
    ImGui::SameLine(300.0f);
    ImGui::Text("%s", value ? value : "Unknown");
}

static void InfoRow(const char* label, int value) {
    ImGui::Text("%s", label);
    ImGui::SameLine(300.0f);
    ImGui::Text("%d", value);
}

static void InfoRow(const char* label, float value, const char* format = "%.1f") {
    ImGui::Text("%s", label);
    ImGui::SameLine(300.0f);
    ImGui::Text(format, value);
}

static void InfoRow(const char* label, bool value) {
    ImGui::Text("%s", label);
    ImGui::SameLine(300.0f);
    ImGui::TextUnformatted(value ? "Yes" : "No");
}

namespace UI {
    SettingsMenu::SettingsMenu() {
        AudioEngine* audio = Application::getContext()->audioEngine;
        audio->loadDevices();
        m_outputDevices = audio->getOutputDevices();

        m_unlimitedFrameRate = Application::getContext()->instance->gameSettings.graphics.maxFramerate == 0;
    }

    void SettingsMenu::render() {
        ApplicationContext* context = Application::getContext();
        GameSettings& settings = context->instance->gameSettings;
        Renderer* renderer = context->renderer;

        ImGuiIO& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        UI::Util::MakeNextWindowFullscreen();
        ImGui::Begin("Settings", nullptr, window_flags);
        {
            ScopedFont font(UI::manager().getFont(25));

            // Reserve space for bottom button.
            const float buttonHeight = 45.0f;
            const float padding = 20.0f;
            const float bottomAreaHeight = buttonHeight + padding * 2.0f;

            // Everything above the button.
            ImGui::BeginChild("SettingsContent", ImVec2(0.0f, -bottomAreaHeight), false);

            if (ImGui::BeginTabBar("SettingsTabs")) {
                if (ImGui::BeginTabItem("Graphics")) {
                    SettingsSection("Display");

                    if (ImGui::Combo(
                            "Window mode", &settings.graphics.windowMode, windowModes, IM_ARRAYSIZE(windowModes)
                        )) {
                        context->windowManager->setWindowMode(static_cast<WindowMode>(settings.graphics.windowMode));
                    }
                    if (ImGui::Checkbox("VSync", &settings.graphics.vsync)) {
                        context->windowManager->enableVSync(settings.graphics.vsync);
                    }
                    if (ImGui::Checkbox("Unlimited FPS", &m_unlimitedFrameRate)) {
                        settings.graphics.maxFramerate = m_unlimitedFrameRate ? 0 : 90;
                    }
                    if (!m_unlimitedFrameRate) {
                        ImGui::SliderInt("Max Framerate", &settings.graphics.maxFramerate, 30, 300);
                    }

                    SettingsSection("World");

                    if (ImGui::SliderInt("Render Distance", &settings.graphics.renderDistance, 2, 32)) {
                        if (context->instance->m_world) {
                            context->instance->m_world->setChunkLoadingDistance(settings.graphics.renderDistance);
                        }
                    }

                    ImGui::SliderInt("Simulation Distance", &settings.graphics.simulationDistance, 2, 32);

                    float renderScalePercent = settings.graphics.renderScale * 100.0f;
                    if (ImGui::SliderFloat("Render Scale", &renderScalePercent, 25.00f, 100.0f, "%.0f%%")) {
                        settings.graphics.renderScale = renderScalePercent / 100.0f;
                    }

                    ImGui::SliderFloat("FOV", &settings.graphics.fieldOfView, 50.0f, 110.0f, "%.0f°");

                    SettingsSection("Visuals");

                    ImGui::Checkbox("Entity Shadows", &settings.graphics.entityShadows);
                    ImGui::Checkbox("Particles", &settings.graphics.particles);
                    ImGui::Checkbox("Clouds", &settings.graphics.clouds);

                    if (ImGui::Checkbox("Ambient Occlusion [SSAO]", &settings.graphics.ambientOcclusion)) {
                        context->renderer->getPass<SSAORenderpass>()->setEnabled(settings.graphics.ambientOcclusion);
                    }
                    if (ImGui::Checkbox("Anti Aliasing [FXAA]", &settings.graphics.fxaa)) {
                        context->renderer->getPass<FXAARenderpass>()->setEnabled(settings.graphics.fxaa);
                    }

                    SettingsSection("Quality");

                    ImGui::Combo(
                        "Shadow Quality", &settings.graphics.shadowQuality, qualityNames, IM_ARRAYSIZE(qualityNames)
                    );
                    ImGui::Combo(
                        "Texture Quality", &settings.graphics.textureQuality, qualityNames, IM_ARRAYSIZE(qualityNames)
                    );
                    ImGui::Combo(
                        "Particle Quality", &settings.graphics.particleQuality, qualityNames, IM_ARRAYSIZE(qualityNames)
                    );
                    SettingsSection("Debug");
                    if (ImGui::Checkbox("Polygon Rendermode", &settings.graphics.debugPolygonModeEnabled)) {
                        context->renderer->setDebugPolygonModeEnabled(settings.graphics.debugPolygonModeEnabled);
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Audio")) {
                    if (ImGui::SliderFloat("Master Volume", &settings.audio.masterVolume, 0.0f, 1.0f)) {
                        context->audioEngine->setBusVolume(MASTER_BUS, settings.audio.masterVolume);
                    }

                    if (ImGui::SliderFloat("Music Volume", &settings.audio.musicVolume, 0.0f, 1.0f)) {
                        context->audioEngine->setBusVolume(MUSIC_BUS, settings.audio.musicVolume);
                    }

                    if (ImGui::SliderFloat("Effect Volume", &settings.audio.effectVolume, 0.0f, 1.0f)) {
                        context->audioEngine->setBusVolume(EFFECT_BUS, settings.audio.effectVolume);
                    }

                    if (ImGui::SliderFloat("Ambient Volume", &settings.audio.ambientVolume, 0.0f, 1.0f)) {
                        context->audioEngine->setBusVolume(AMBIENT_BUS, settings.audio.ambientVolume);
                    }

                    ImGui::Spacing();
                    ImGui::Spacing();

                    const char* preview = "Default";
                    for (const AudioDevice& device : m_outputDevices) {
                        if (device.name == settings.audio.outputDevice) {
                            preview = device.name.c_str();
                            break;
                        }
                    }

                    if (ImGui::BeginCombo("Output Device", preview)) {
                        for (const AudioDevice& device : m_outputDevices) {
                            std::string label = device.name;
                            if (device.isDefault) label += " (Default)";

                            bool selected = device.name == settings.audio.outputDevice;
                            if (ImGui::Selectable(label.c_str(), selected)) {
                                settings.audio.outputDevice = device.name;
                                context->audioEngine->setOutputDevice(device.id);
                            }

                            if (selected) ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Controls")) {
                    // Controls here...

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Gameplay")) {
                    // Gameplay settings...

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Interface")) {
                    // Interface settings...

                    ImGui::EndTabItem();
                }

                ImGui::Spacing();

                if (ImGui::BeginTabItem("System Information")) {
                    const GraphicsInfo& graphicsInfo = renderer->getGraphicsInfo();
                    const GraphicsContextInfo& contextInfo = graphicsInfo.context;
                    const GraphicsLimits& limits = graphicsInfo.limits;

                    SettingsSection("Graphics");

                    InfoRow("Vendor", graphicsInfo.vendor);
                    InfoRow("Renderer", graphicsInfo.renderer);
                    InfoRow("OpenGL Version", graphicsInfo.version);
                    InfoRow("GLSL Version", graphicsInfo.shadingLanguageVersion);

                    SettingsSection("OpenGL Context");

                    std::string contextVersion = std::to_string(contextInfo.majorVersion) + "." +
                                                 std::to_string(contextInfo.minorVersion);

                    InfoRow("Version", contextVersion);

                    std::string profile;

                    if (contextInfo.coreProfile) {
                        profile = "Core";
                    } else if (contextInfo.compatibilityProfile) {
                        profile = "Compatibility";
                    } else {
                        profile = "Unknown";
                    }

                    InfoRow("Profile", profile);
                    InfoRow("Debug Context", contextInfo.debugContext);
                    InfoRow("Forward Compatible", contextInfo.forwardCompatible);
                    InfoRow("Robust Access", contextInfo.robustAccess);

                    ImGui::Spacing();
                    ImGui::Spacing();
                    
                    if (ImGui::CollapsingHeader("Advanced Limits")) {
                        SettingsSection("Texture Limits");

                        InfoRow("Max Texture Size", limits.maxTextureSize);
                        InfoRow("Max 3D Texture Size", limits.max3DTextureSize);
                        InfoRow("Max Cube Map Size", limits.maxCubeMapTextureSize);
                        InfoRow("Max Array Texture Layers", limits.maxArrayTextureLayers);
                        InfoRow("Max Texture Units", limits.maxTextureImageUnits);
                        InfoRow("Max Combined Texture Units", limits.maxCombinedTextureImageUnits);
                        InfoRow("Max Vertex Texture Units", limits.maxVertexTextureImageUnits);

                        SettingsSection("Rendering Limits");

                        InfoRow("Max Vertex Attributes", limits.maxVertexAttribs);
                        InfoRow("Max Draw Buffers", limits.maxDrawBuffers);
                        InfoRow("Max Color Attachments", limits.maxColorAttachments);
                        InfoRow("Max MSAA Samples", limits.maxSamples);
                        InfoRow("Max Viewport Width", limits.maxViewportWidth);
                        InfoRow("Max Viewport Height", limits.maxViewportHeight);
                        InfoRow("Max Anisotropy", limits.maxAnisotropy, "%.1fx");

                        SettingsSection("Buffer Limits");

                        InfoRow("Max Uniform Buffer Bindings", limits.maxUniformBufferBindings);
                        InfoRow("Max Uniform Block Size", limits.maxUniformBlockSize);
                        InfoRow("Max SSBO Bindings", limits.maxShaderStorageBufferBindings);
                        InfoRow("Max SSBO Block Size", limits.maxShaderStorageBlockSize);

                        SettingsSection("Shader Limits");

                        InfoRow("Vertex Uniform Components", limits.maxVertexUniformComponents);
                        InfoRow("Fragment Uniform Components", limits.maxFragmentUniformComponents);
                        InfoRow("Max Varying Vectors", limits.maxVaryingVectors);

                        SettingsSection("Compute Shader Limits");

                        InfoRow("Max Work Group Invocations", limits.maxComputeWorkGroupInvocations);

                        std::string workGroupCount = std::to_string(limits.maxComputeWorkGroupCount[0]) + " x " +
                                                     std::to_string(limits.maxComputeWorkGroupCount[1]) + " x " +
                                                     std::to_string(limits.maxComputeWorkGroupCount[2]);

                        std::string workGroupSize = std::to_string(limits.maxComputeWorkGroupSize[0]) + " x " +
                                                    std::to_string(limits.maxComputeWorkGroupSize[1]) + " x " +
                                                    std::to_string(limits.maxComputeWorkGroupSize[2]);

                        InfoRow("Max Work Group Count", workGroupCount);
                        InfoRow("Max Work Group Size", workGroupSize);
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndChild();

            // --------------------------------------------------
            // Bottom area
            // --------------------------------------------------

            ImGui::Dummy(ImVec2(0.0f, padding));

            const float buttonWidth = 350.0f;

            float centerX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerX);

            if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
                navigateBack();
            }
        }

        ImGui::End();
    }
}  // namespace UI
