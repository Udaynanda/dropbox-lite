#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>

namespace dropboxlite {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& instance();
    
    void setLevel(LogLevel level);
    void setLogFile(const std::string& filepath);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
private:
    Logger() = default;
    void log(LogLevel level, const std::string& message);
    
    LogLevel level_ = LogLevel::INFO;
    std::mutex mutex_;
    std::unique_ptr<std::ofstream> file_;
};

// Convenience macros
#define LOG_DEBUG(msg) dropboxlite::Logger::instance().debug(msg)
#define LOG_INFO(msg) dropboxlite::Logger::instance().info(msg)
#define LOG_WARNING(msg) dropboxlite::Logger::instance().warning(msg)
#define LOG_ERROR(msg) dropboxlite::Logger::instance().error(msg)

} // namespace dropboxlite
