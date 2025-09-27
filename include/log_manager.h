#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ELRS
{
    /**
     * Log level enumeration
     */
    enum class LogLevel
    {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };

    /**
     * Log entry structure
     */
    struct LogEntry
    {
        std::chrono::steady_clock::time_point timestamp;
        LogLevel level;
        std::string category;
        std::string message;

        std::string getFormattedTime() const
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);

            std::stringstream ss;
            ss << std::put_time(&tm, "%H:%M:%S");
            return ss.str();
        }

        std::string getLevelString() const
        {
            switch (level)
            {
            case LogLevel::Debug:
                return "DEBUG";
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warning:
                return "WARN";
            case LogLevel::Error:
                return "ERROR";
            default:
                return "UNKNOWN";
            }
        }
    };

    /**
     * Thread-safe logging manager
     */
    class LogManager
    {
    public:
        static LogManager &getInstance();

        void log(LogLevel level, const std::string &category, const std::string &message);
        void debug(const std::string &category, const std::string &message);
        void info(const std::string &category, const std::string &message);
        void warning(const std::string &category, const std::string &message);
        void error(const std::string &category, const std::string &message);

        std::vector<LogEntry> getRecentLogs(size_t maxCount = 100) const;
        size_t getLogCount() const;
        void clearLogs();

        void setLogLevel(LogLevel minLevel);
        LogLevel getLogLevel() const;

    private:
        LogManager() = default;
        ~LogManager() = default;
        LogManager(const LogManager &) = delete;
        LogManager &operator=(const LogManager &) = delete;

        mutable std::mutex logs_mutex_;
        std::vector<LogEntry> logs_;
        LogLevel min_log_level_ = LogLevel::Info;
        static const size_t MAX_LOG_ENTRIES = 1000;
    };

// Convenience macros for logging
#define LOG_DEBUG(category, message) ELRS::LogManager::getInstance().debug(category, message)
#define LOG_INFO(category, message) ELRS::LogManager::getInstance().info(category, message)
#define LOG_WARNING(category, message) ELRS::LogManager::getInstance().warning(category, message)
#define LOG_ERROR(category, message) ELRS::LogManager::getInstance().error(category, message)

} // namespace ELRS