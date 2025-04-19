#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>

enum LogLevel { DEBUG, INFO, WARNING, ERROR, FATAL };

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& instance();
    
    void setLevel(LogLevel level);
    void log(LogLevel level, const std::string& message);
    std::atomic<LogLevel> currentLevel;
private:
    Logger();  // 默认构造私有化
    std::mutex logMutex;
};

class LogStream : public std::ostringstream {
public:
    explicit LogStream(LogLevel level);
    ~LogStream();

private:
    LogLevel level;
};

#define LOG(level) LogStream(level)