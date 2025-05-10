#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>
#include <fstream>

const std::string logPath = "/var/log/httplog.txt";

enum LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger
{
public:
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static Logger &instance();

    void setLevel(LogLevel level);
    void log(LogLevel level, const std::string &message);
    void logToFile(const std::string &message);
    std::atomic<LogLevel> currentLevel;

    void setLogToScreen();
    void setLogToFile();

private:
    Logger(); // 默认构造私有化
    std::mutex logMutex;
    bool saveToFile;
};

class LogStream : public std::ostringstream
{
public:
    explicit LogStream(LogLevel level);
    ~LogStream();

private:
    LogLevel level;
};

#define LOG(level) LogStream(level)

#define LOGTOSCREEN() Logger::instance().setLogToScreen()

#define LOGTOFILE() Logger::instance().setLogToFile()