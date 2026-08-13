#include "Manager.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppConstants.h"

UI::Manager::Manager() : m_currentWidget(nullptr), m_nextWidget(nullptr) {}

UI::Manager::~Manager() {
    if (m_currentWidget) {
        delete m_currentWidget;
    }
    if (m_nextWidget) {
        delete m_nextWidget;
    }
}

void UI::Manager::init() {
    // ImGui setup
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;  // Weird 1px border otherwise
    ImGui::StyleColorsDark();

    // Load font with sizes used for interpolation
    m_fontPool.loadFontSizes(Res::Font::PROGGY_CLEAN, {16.0f, 32.0f, 48.0f, 64.0f});
}

void UI::Manager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::Manager::renderFrame() {
    if (m_nextWidget) {
        // Navigate safely to new window
        if (m_currentWidget) {
            delete m_currentWidget;
        }
        m_currentWidget = m_nextWidget;
        m_nextWidget = nullptr;

        m_currentWidgetName = m_nextWidgetName;
        m_nextWidgetName.clear();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_currentWidget) {
        m_currentWidget->render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::Manager::registerWidget(const std::string& widgetName, std::function<Widget*()> createFn) {
    m_widgetFactory.emplace(widgetName, createFn);
}

bool UI::Manager::navigateTo(const std::string& widgetName) {
    auto it = m_widgetFactory.find(widgetName);
    if (it == m_widgetFactory.end() || m_nextWidget) return false;

    if (!m_currentWidgetName.empty()) {
        m_navigationStack.push_back(m_currentWidgetName);
    }

    m_nextWidget = it->second();
    m_nextWidgetName = widgetName;
    return true;
}

bool UI::Manager::navigateToReplacement(const std::string& widgetName) {
    auto it = m_widgetFactory.find(widgetName);
    if (it == m_widgetFactory.end() || m_nextWidget) return false;

    m_nextWidget = it->second();
    m_nextWidgetName = widgetName;
    return true;
}

bool UI::Manager::navigateBackTo(const std::string& widgetName) {
    auto it = std::find(m_navigationStack.rbegin(), m_navigationStack.rend(), widgetName);
    if (it == m_navigationStack.rend() || m_nextWidget) return false;

    auto stackIt = std::prev(it.base());
    m_navigationStack.erase(stackIt, m_navigationStack.end());

    auto widgetFactoryIt = m_widgetFactory.find(widgetName);
    if (widgetFactoryIt == m_widgetFactory.end()) return false;

    m_nextWidget = widgetFactoryIt->second();
    m_nextWidgetName = widgetName;
    return true;
}

bool UI::Manager::navigateBack() {
    if (m_navigationStack.empty() || m_nextWidget) return false;

    const std::string previousWidget = std::move(m_navigationStack.back());
    m_navigationStack.pop_back();

    auto it = m_widgetFactory.find(previousWidget);
    if (it == m_widgetFactory.end()) return false;

    m_nextWidget = it->second();
    m_nextWidgetName = previousWidget;
    return true;
}

void UI::Manager::clearNavigationHistory() { m_navigationStack.clear(); }

FontData UI::Manager::getFont(float requestedSize) { return m_fontPool.getFont(requestedSize); }
