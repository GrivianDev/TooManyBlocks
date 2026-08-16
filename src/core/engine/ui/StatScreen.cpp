#include "StatScreen.h"

#include <imgui.h>

#include <glm/vec3.hpp>

#include "Application.h"
#include "datatypes/Transform.h"
#include "engine/GameInstance.h"
#include "engine/controllers/PlayerController.h"
#include "engine/entity/Entity.h"
#include "engine/rendering/Renderer.h"
#include "engine/ui/UiUtil.h"
#include "foundation/util/PrettyPrint.h"
#include "foundation/time/Timer.h"

void UI::StatScreen::render() {
    ApplicationContext* context = Application::getContext();

    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    // Position: top-right corner
    ImVec2 windowPos(io.DisplaySize.x, 0);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(450, 0), ImVec2(450, FLT_MAX));
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Stat Screen", nullptr, window_flags);
    {
        ScopedFont font(UI::manager().getFont(18));

        Transform& cameraTransform = context->instance->m_player->getCamera()->getLocalTransform();
        glm::vec3 camRotation = cameraTransform.getRotationEuler();
        glm::vec3 camForward = cameraTransform.getForward();
        glm::vec3 playerPos = context->instance->m_player->getTransform().getPosition();
        glm::vec3 velocity = context->instance->m_player->getVelocity();

        ImGui::SeparatorText("Application");
        ImGui::Text("%.1f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("%.2f %% CPU Usage", context->stats.cpuUsage * 100.0f);
        ImGui::Text(
            "%s / %s RAM System Memory",
            formatBytes(context->stats.memInfo.bytesInUse, ByteUnit::Bytes),
            formatBytes(context->stats.memInfo.totalBytes, ByteUnit::Bytes)
        );
        ImGui::Text("%s allocated by this process", formatBytes(context->stats.processUsedBytes, ByteUnit::Bytes));
        ImGui::Text(
            "%s read | %lu read calls",
            formatBytes(context->stats.processIo.bytesRead, ByteUnit::Bytes),
            context->stats.processIo.readCalls
        );
        ImGui::Text(
            "%s written | %lu write calls",
            formatBytes(context->stats.processIo.bytesWritten, ByteUnit::Bytes),
            context->stats.processIo.writeCalls
        );

        m_renderDebugReportAccumulator += context->timer->deltaSeconds();
        if (m_renderDebugReportAccumulator > 0.1f) {
            m_renderDebugReportAccumulator = 0.0f;
            m_report.clear();
            context->renderer->fillDebugReport(m_report);
        }
        UI::Util::DrawDebugNode(m_report.getRoot());

        ImGui::SeparatorText("World Info");
        ImGui::Text("Objects in scene: %lu", context->instance->m_world->scene().objectCount());
        ImGui::Text("Loaded chunks: %lu", context->instance->m_world->loadedChunks().size());
        glm::ivec3 currChunkOrigin = Chunk::worldToChunkOrigin(playerPos);
        ImGui::Text("Current chunk origin: (%d,%d,%d)", currChunkOrigin.x, currChunkOrigin.y, currChunkOrigin.z);

        ImGui::SeparatorText("Player");
        ImGui::Text("Player Position: x=%.1f, y=%.1f, z=%.1f", playerPos.x, playerPos.y, playerPos.z);
        ImGui::Text("Velocity: x=%.1f, y=%.1f, z=%.1f", velocity.x, velocity.y, velocity.z);
        ImGui::Text("Cam Rotation: x=%.1f, y=%.1f, z=%.1f", camRotation.x, camRotation.y, camRotation.z);
        ImGui::Text("Cam Forward Vec: x=%.1f, y=%.1f, z=%.1f", camForward.x, camForward.y, camForward.z);

        // Get camera orientation vectors
        glm::quat camQuat = context->instance->m_player->getCamera()->getLocalTransform().getRotationQuat();
        glm::vec3 x = glm::vec3(1, 0, 0) * camQuat;
        glm::vec3 y = glm::vec3(0, 1, 0) * camQuat;
        glm::vec3 z = glm::vec3(0, 0, 1) * camQuat;
        UI::Util::DrawCoordinateSystem(x, y, z, 125.0f, 50.0f);
    }
    ImGui::End();
}