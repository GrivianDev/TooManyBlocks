#include "Logger.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// ANSI escape codes for colors
static const std::string RED = "\033[31m";
static const std::string GREEN = "\033[32m";
static const std::string YELLOW = "\033[33m";
static const std::string BLUE = "\033[34m";
static const std::string RESET = "\033[0m";

namespace lgr {
    Logger lout;

    std::string Logger::getCurrentTime() const noexcept {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void Logger::log(const LogLevel& llevel, const std::string& msg) noexcept {
        std::string tag;
        std::string color;

        switch (llevel) {
            case DEBUG:
                color = GREEN;
                tag = "DEBUG";
                break;
            case INFO:
                color = BLUE;
                tag = "INFO";
                break;
            case WARNING:
                color = YELLOW;
                tag = "WARNING";
                break;
            case ERR:
                color = RED;
                tag = "ERROR";
                break;
        }

        if (m_consoleAvailable) {
            tag = color + tag + RESET;
        }

        std::string timeStampedMessage = "[" + getCurrentTime() + "] [" + tag + "] " + msg;

        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_consoleAvailable) {
            std::cout << timeStampedMessage << std::endl;
        } else if (m_logFile.is_open()) {
            m_logFile << timeStampedMessage << std::endl;
        }
    }

    Logger::Logger() noexcept {
#ifdef _WIN32
        // Check if console is available on windows
        m_consoleAvailable = GetConsoleWindow() != nullptr;
        if (m_consoleAvailable) {
            // Try and configure console for colored text ouput
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode)) {
                    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hOut, dwMode);
                };
            }
        }
#else
        // Check if console is available on other platform
        m_consoleAvailable = isatty(fileno(stdout));
#endif
        if (!m_consoleAvailable) {
            m_logFile.open("log.txt", std::ios::out | std::ios::trunc);
        }
    }

    Logger::~Logger() noexcept {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

    void lgr::Logger::debug(const std::string& msg) noexcept { log(Logger::LogLevel::DEBUG, msg); }

    void lgr::Logger::info(const std::string& msg) noexcept { log(Logger::LogLevel::INFO, msg); }

    void lgr::Logger::warn(const std::string& msg) noexcept { log(Logger::LogLevel::WARNING, msg); }

    void lgr::Logger::error(const std::string& msg) noexcept { log(Logger::LogLevel::ERR, msg); }
}  // namespace lgr