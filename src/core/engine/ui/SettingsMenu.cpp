#include "SettingsMenu.h"

#include <imgui.h>

#include "Application.h"
#include "engine/GameInstance.h"
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

namespace UI {
    SettingsMenu::SettingsMenu() {
        AudioEngine* audio = Application::getContext()->audioEngine;
        audio->loadDevices();
        m_outputDevices = audio->getOutputDevices();
    }

    void SettingsMenu::render() {
        ApplicationContext* context = Application::getContext();
        GameSettings& settings = context->instance->gameSettings;

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

                    ImGui::SliderInt("Max Framerate", &settings.graphics.maxFramerate, 30, 300);

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

                    ImGui::Checkbox("Ambient Occlusion", &settings.graphics.ambientOcclusion);

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
                navigateToWidget("MainMenu");
            }
        }

        ImGui::End();
    }
}  // namespace UI
