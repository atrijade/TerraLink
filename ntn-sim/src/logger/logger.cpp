#include "ntn-sim/logger/logger.h"
#include <iostream>

namespace NtnSim::Logger {

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::Initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(logFilePath, std::ios::out | std::ios::app);
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string levelStr = LogLevelToString(level);
    std::string logLine = "[" + levelStr + "] " + message;

    // Output to console
    std::cout << logLine << std::endl;

    // Output to file if available
    if (m_fileStream.is_open()) {
        m_fileStream << logLine << std::endl;
    }
}

void Logger::SetMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

std::string Logger::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

} // namespace NtnSim::Logger
