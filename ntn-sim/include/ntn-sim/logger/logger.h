#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <memory>

namespace NtnSim::Logger {

/**
 * @brief Logging levels.
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

/**
 * @brief Thread-safe Singleton Logger.
 */
class Logger {
public:
    /**
     * @brief Gets the singleton instance of the Logger.
     */
    static Logger& GetInstance();

    // Delete copy/move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Initializes logger targets.
     * @param logFilePath Path to the log file on disk.
     */
    void Initialize(const std::string& logFilePath);

    /**
     * @brief Logs a message with specified severity level.
     * @param level Severity level.
     * @param message Text message.
     */
    void Log(LogLevel level, const std::string& message);

    /**
     * @brief Sets the minimum logging level.
     */
    void SetMinLevel(LogLevel level);

private:
    Logger();
    ~Logger();

    std::string LogLevelToString(LogLevel level) const;

    LogLevel m_minLevel{LogLevel::INFO};
    std::ofstream m_fileStream;
    std::mutex m_mutex;
};

} // namespace NtnSim::Logger
