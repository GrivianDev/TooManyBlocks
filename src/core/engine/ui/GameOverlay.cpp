#include "GameOverlay.h"

#include <imgui.h>

#include "Application.h"
#include "engine/GameInstance.h"
#include "engine/controllers/PlayerController.h"
#include "engine/entity/Entity.h"

void UI::GameOverlay::render() {
    ApplicationContext* context = Application::getContext();

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) {
        m_showStats = !m_showStats;
    }

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar;

    UI::Util::MakeNextWindowFullscreen();
    ImGui::Begin("Game Overlay", nullptr, window_flags);
    {
        ScopedFont font(UI::manager().getFont(20));

        if (m_showStats) {
            m_statScreen.render();
        }

        ImVec2 pos(io.DisplaySize.x - 110, 10);  // Adjust X for width
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::PushItemWidth(200);
        ImGui::SetNextWindowBgAlpha(0.0f);  // transparent background

        if (ImGui::BeginCombo("##Dropdown", "Movement Mode", ImGuiComboFlags_NoArrowButton)) {
            if (ImGui::Selectable("Walk")) {
                context->instance->m_player->getMovementComponent()->setMovementMode(MovementMode::Walk);
            }
            if (ImGui::Selectable("Fly")) {
                context->instance->m_player->getMovementComponent()->setMovementMode(MovementMode::Fly);
            }
            if (ImGui::Selectable("Spectator")) {
                context->instance->m_player->getMovementComponent()->setMovementMode(MovementMode::Spectator);
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        UI::Util::DrawCrosshair(35.0f, 3.0f);

        bool newShowMouse = ImGui::IsKeyDown(ImGuiKey_LeftAlt);
        if (newShowMouse != m_showMouse) {
            m_showMouse = newShowMouse;
            context->windowManager->setCursorMode(m_showMouse ? CursorMode::Normal : CursorMode::HiddenAndCaptured);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            context->instance->gameState.gamePaused = true;
            context->inputManager->keyAdapter()->detach(
                static_cast<PlayerController*>(context->instance->m_player->getController())
            );
            context->inputManager->mouseAdapter()->detach(
                static_cast<PlayerController*>(context->instance->m_player->getController())
            );
            context->windowManager->setCursorMode(CursorMode::Normal);
            navigateTo("PauseMenu");
        }
    }
    ImGui::End();
}