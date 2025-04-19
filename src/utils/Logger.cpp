#include "../../include/utils/Logger.h"
#include <iostream>

Logger::Logger() : currentLevel(INFO) {} // 构造初始化原子变量

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    currentLevel.store(level);
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < currentLevel.load()) return;
    
    const char* levelNames[] = {"DEBUG", "INFO","WARNING","ERROR","FATAL"};
    
    std::lock_guard<std::mutex> lock(logMutex);
    std::cerr << "[" << levelNames[static_cast<int>(level)] << "] " << msg << '\n';
}

LogStream::LogStream(LogLevel level) : level(level) {}

LogStream::~LogStream() {
    auto currentLevel = Logger::instance().currentLevel.load();
    if (level >= currentLevel) {
        Logger::instance().log(level, this->str());
    }
}