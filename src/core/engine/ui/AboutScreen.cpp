#include "AboutScreen.h"

#include <imgui.h>

#include <fstream>

#include "Application.h"
#include "Logger.h"
#include "foundation/util/Utility.h"

namespace UI {
    void AboutScreen::startLoadingContent() {
        m_content = Future<std::string>([]() {
            return readFile(std::string("third_party.txt"));
        });

        m_content.start();
    }

    void AboutScreen::render() {
        ApplicationContext* context = Application::getContext();

        if (m_content.isEmpty()) {
            startLoadingContent();
        }
        if (m_content.hasError()) {
            try {
                std::rethrow_exception(m_content.getException());
            } catch(const std::exception& e) {
                setError(e.what());
            }
            m_content.reset();
        }

        ImGuiIO& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        UI::Util::MakeNextWindowFullscreen();
        ImGui::Begin("About", NULL, window_flags);
        {
            ScopedFont font(UI::manager().getFont(25));
            char aboutTxt[32];
            std::sprintf(aboutTxt, "About %s", APP_NAME);
            ImVec2 titleSize = ImGui::CalcTextSize(aboutTxt);

            ImGui::SetCursorPosX((io.DisplaySize.x - titleSize.x) * 0.5f);  // Center horizontally
            ImGui::Text("%s", aboutTxt);
            ImGui::Dummy(ImVec2(0, 20));

            float availableHeight = ImGui::GetContentRegionAvail().y;
            float buttonHeight = 45.0f;
            float padding = 20.0f;  // Space between text and button
            float textHeight = availableHeight - buttonHeight - padding - padding;
            ImGui::BeginChild("TextRegion", ImVec2(0, textHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
            if (ImGui::CollapsingHeader("Third Party Licenses")) {
                if (hasError()) {
                    float errorWidth = ImGui::CalcTextSize(getError()).x;
                    ImGui::SetCursorPosX((io.DisplaySize.x - errorWidth) * 0.5f);
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", getError());
                } else {
                    if (!m_content.isReady()) {
                        ImGui::Text("Loading...");
                    } else {
                        ScopedFont contentFont(UI::manager().getFont(20));
                        ImGui::TextUnformatted(m_content.value().c_str());
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0, padding));

            float buttonWidth = 350.0f;
            float centerX = (io.DisplaySize.x - buttonWidth) * 0.5f;
            ImGui::SetCursorPosX(centerX);
            if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
                navigateBack();
            }
        }
        ImGui::End();
    }
}  // namespace UI
