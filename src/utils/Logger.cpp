#include "../../include/utils/Logger.h"
#include <iostream>

Logger::Logger() : currentLevel(INFO), saveToFile(false) {} // 构造初始化原子变量

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level)
{
    currentLevel.store(level);
}

void Logger::log(LogLevel level, const std::string &msg)
{
    // 如果等级低于最小级别，直接忽略
    if (level < currentLevel.load())
        return;

    const char *levelNames[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

    switch (saveToFile)
    {
    case true:
        // to do
        //写入到文件
        logToFile(msg);
        break;
    case false:
        // to do
        //直接打印到标准错误
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[" << levelNames[static_cast<int>(level)] << "] " << msg << '\n';
        break;
    }
}

void Logger::logToFile(const std::string &message)
{ 
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::ofstream out(logPath,std::ios::app);
    if(!out.is_open())
    {
        return;
    }
    out<<message<<std::endl;
    //out.write(message.c_str(),message.size());
    out.close();
}


void Logger::setLogToFile()
{
    saveToFile = true;
}

void Logger::setLogToScreen()
{
    saveToFile = false;
}

LogStream::LogStream(LogLevel level) : level(level) {}

LogStream::~LogStream()
{
    auto currentLevel = Logger::instance().currentLevel.load();
    if (level >= currentLevel)
    {
        Logger::instance().log(level, this->str());
    }
}