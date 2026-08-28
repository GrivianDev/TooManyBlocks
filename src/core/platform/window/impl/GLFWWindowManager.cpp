#include "GLFWWindowManager.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

#include "Application.h"

static void windowResizeCallback(GLFWwindow* window, int width, int height) {
    ApplicationContext* context = Application::getContext();
    context->state.screenWidth = static_cast<unsigned int>(width);
    context->state.screenHeight = static_cast<unsigned int>(height);
}

GLFWWindowManager::GLFWWindowManager()
    : m_currentWindow(nullptr), m_currentMode(WindowMode::Windowed), m_isInitialized(false) {}

GLFWWindowManager::~GLFWWindowManager() { shutdown(); }

void GLFWWindowManager::init() {
    if (m_isInitialized) {
        throw std::runtime_error("Duplicate initialization!");
    }

    if (glfwInit() != GLFW_TRUE) throw std::runtime_error("Error initializing glfw!");

    // Specifiy OpenGl Version to use
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Core does not auto create vertex array objects, compatibitly does
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef DEBUG_MODE  // Create debug open gl context (Some graphics cards do not enable debugging by default)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
}

void GLFWWindowManager::shutdown() {
    if (m_isInitialized) {
        // This implicitly destroys open gl context -> gl calls afterwards will cause error
        if (m_currentWindow) {
            glfwDestroyWindow(m_currentWindow);
        }
        glfwTerminate();
    }
}

void GLFWWindowManager::createActiveWindow(unsigned int width, unsigned int height, const char* title) {
    m_currentWindow = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetFramebufferSizeCallback(m_currentWindow, windowResizeCallback);
    glfwMakeContextCurrent(m_currentWindow);  // Set active (Also activates OpenGL context on this thread)
}

bool GLFWWindowManager::shouldWindowClose() const { return glfwWindowShouldClose(m_currentWindow); }

void GLFWWindowManager::setWindowSize(unsigned int width, unsigned int height) {
    glfwSetWindowSize(m_currentWindow, static_cast<int>(width), static_cast<int>(height));
}

void GLFWWindowManager::setWindowPosition(int x, int y) { glfwSetWindowPos(m_currentWindow, x, y); }

void GLFWWindowManager::swapBuffers() { glfwSwapBuffers(m_currentWindow); }

void GLFWWindowManager::pollEvents() { glfwPollEvents(); }

void GLFWWindowManager::enableVSync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }

void GLFWWindowManager::setCursorMode(CursorMode mode) {
#ifdef DEBUG_MODE
    if (mode == CursorMode::HiddenAndCaptured) /* Prevent debug point triggering with captured mouse*/ return;
#endif

    switch (mode) {
        case CursorMode::Normal: glfwSetInputMode(m_currentWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL); break;
        case CursorMode::HiddenAndCaptured: glfwSetInputMode(m_currentWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED); break;
        default: throw std::runtime_error("Unspecified cursor mode");
    }
}

std::vector<MonitorInfo> GLFWWindowManager::getAvailableMonitors() const {
    std::vector<MonitorInfo> result;

    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    result.reserve(count);

    for (int i = 0; i < count; i++) {
        GLFWmonitor* monitor = monitors[i];

        int x, y;
        glfwGetMonitorPos(monitor, &x, &y);

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        MonitorInfo info;
        info.id = i;
        info.x = x;
        info.y = y;
        info.width = static_cast<unsigned int>(mode->width);
        info.height = static_cast<unsigned int>(mode->height);
        info.refreshRate = mode->refreshRate;
        info.name = glfwGetMonitorName(monitor);

        result.push_back(info);
    }

    return result;
}

void GLFWWindowManager::setWindowMode(WindowMode mode, int monitorId) {
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    if (monitorCount == 0) return;

    GLFWmonitor* monitor = nullptr;
    if (monitorId >= 0 && monitorId < monitorCount) {
        monitor = monitors[monitorId];
    } else {
        monitor = glfwGetPrimaryMonitor();
    }

    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

    // Save windowed state before leaving windowed mode
    if (m_currentMode == WindowMode::Windowed) {
        glfwGetWindowPos(m_currentWindow, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_currentWindow, &m_windowedWidth, &m_windowedHeight);
    }

    switch (mode) {
        case WindowMode::Windowed: {
            glfwSetWindowAttrib(m_currentWindow, GLFW_DECORATED, GLFW_TRUE);
            glfwSetWindowMonitor(
                m_currentWindow, nullptr, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight, GLFW_DONT_CARE
            );
            break;
        }

        case WindowMode::Borderless: {
            int monitorX, monitorY;
            glfwGetMonitorPos(monitor, &monitorX, &monitorY);

            glfwSetWindowAttrib(m_currentWindow, GLFW_DECORATED, GLFW_FALSE);
            glfwSetWindowMonitor(
                m_currentWindow, nullptr, monitorX, monitorY, videoMode->width, videoMode->height, GLFW_DONT_CARE
            );
            break;
        }

        case WindowMode::Fullscreen: {
            glfwSetWindowAttrib(m_currentWindow, GLFW_DECORATED, GLFW_TRUE);
            glfwSetWindowMonitor(
                m_currentWindow, monitor, 0, 0, videoMode->width, videoMode->height, videoMode->refreshRate
            );
            break;
        }
    }

    m_currentMode = mode;
}
