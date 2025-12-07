#include "common/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace dropboxlite {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_ = std::make_unique<std::ofstream>(filepath, std::ios::app);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    // Level string
    const char* level_str;
    switch (level) {
        case LogLevel::DEBUG:   level_str = "DEBUG"; break;
        case LogLevel::INFO:    level_str = "INFO"; break;
        case LogLevel::WARNING: level_str = "WARN"; break;
        case LogLevel::ERROR:   level_str = "ERROR"; break;
    }
    
    std::string log_line = ss.str() + " [" + level_str + "] " + message;
    
    // Output to console
    std::cout << log_line << std::endl;
    
    // Output to file if configured
    if (file_ && file_->is_open()) {
        *file_ << log_line << std::endl;
        file_->flush();
    }
}

} // namespace dropboxlite
