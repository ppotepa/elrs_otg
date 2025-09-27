#include "log_manager.h"
#include <iostream>

namespace ELRS
{
    LogManager &LogManager::getInstance()
    {
        static LogManager instance;
        return instance;
    }

    void LogManager::log(LogLevel level, const std::string &category, const std::string &message)
    {
        if (level < min_log_level_)
            return;

        std::lock_guard<std::mutex> lock(logs_mutex_);

        LogEntry entry;
        entry.timestamp = std::chrono::steady_clock::now();
        entry.level = level;
        entry.category = category;
        entry.message = message;

        logs_.push_back(entry);

        // Keep only the most recent entries
        if (logs_.size() > MAX_LOG_ENTRIES)
        {
            logs_.erase(logs_.begin(), logs_.begin() + (logs_.size() - MAX_LOG_ENTRIES));
        }
    }

    void LogManager::debug(const std::string &category, const std::string &message)
    {
        log(LogLevel::Debug, category, message);
    }

    void LogManager::info(const std::string &category, const std::string &message)
    {
        log(LogLevel::Info, category, message);
    }

    void LogManager::warning(const std::string &category, const std::string &message)
    {
        log(LogLevel::Warning, category, message);
    }

    void LogManager::error(const std::string &category, const std::string &message)
    {
        log(LogLevel::Error, category, message);
    }

    std::vector<LogEntry> LogManager::getRecentLogs(size_t maxCount) const
    {
        std::lock_guard<std::mutex> lock(logs_mutex_);

        if (logs_.size() <= maxCount)
        {
            return logs_;
        }

        return std::vector<LogEntry>(logs_.end() - maxCount, logs_.end());
    }

    size_t LogManager::getLogCount() const
    {
        std::lock_guard<std::mutex> lock(logs_mutex_);
        return logs_.size();
    }

    void LogManager::clearLogs()
    {
        std::lock_guard<std::mutex> lock(logs_mutex_);
        logs_.clear();
    }

    void LogManager::setLogLevel(LogLevel minLevel)
    {
        min_log_level_ = minLevel;
    }

    LogLevel LogManager::getLogLevel() const
    {
        return min_log_level_;
    }

} // namespace ELRS