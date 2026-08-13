#include "MainMenu.h"

#include <imgui.h>

#include "Application.h"
#include "engine/GameInstance.h"

namespace UI {
    void MainMenu::render() {
        ApplicationContext* context = Application::getContext();

        ImGuiIO& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        UI::Util::MakeNextWindowFullscreen();
        ImGui::Begin("Main Menu", NULL, window_flags);
        {
            ScopedFont font(UI::manager().getFont(55));

            float buttonWidth = 400.0f;
            float buttonHeight = 75.0f;
            float padding = 10.0f;
            float totalHeight = (4 * buttonHeight) + (3 * padding);   // Total height of all buttons + padding
            float startY = (io.DisplaySize.y - totalHeight) / 2.0f;   // Center Y position
            float centerX = (io.DisplaySize.x - buttonWidth) / 2.0f;  // Center X position
            ImGui::SetCursorPos({centerX, startY});
            if (ImGui::Button("Singleplayer", ImVec2(buttonWidth, buttonHeight))) {
                navigateTo("WorldSelection");
            }
            ImGui::SetCursorPos({centerX, startY + buttonHeight + padding});
            if (ImGui::Button("Settings", ImVec2(buttonWidth, buttonHeight))) {
                navigateTo("SettingsMenu");
            }
            ImGui::SetCursorPos({centerX, startY + 2 * (buttonHeight + padding)});
            if (ImGui::Button("About", ImVec2(buttonWidth, buttonHeight))) {
                navigateTo("AboutScreen");
            }
            ImGui::SetCursorPos({centerX, startY + 3 * (buttonHeight + padding)});
            if (ImGui::Button("Quit", ImVec2(buttonWidth, buttonHeight))) {
                context->instance->gameState.quitGame = true;
            }
        }
        ImGui::End();
    }
}  // namespace UI
