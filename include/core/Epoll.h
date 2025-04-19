#pragma once
#include <vector>
#include <sys/epoll.h>

/**
 * @brief Epoll事件管理封装类
 * 
 * 封装epoll系统调用，提供更安全易用的API
 * 支持边缘触发(ET)模式和水平触发(LT)模式
 */
class Epoll {
public:
    static constexpr int MAX_EVENTS = 1024;  // 单次epoll_wait最大事件数
    
    // 构造时创建epoll实例
    Epoll();
    
    // 析构时关闭epoll文件描述符
    ~Epoll();
    
    // 添加文件描述符到epoll监控
    void addFd(int fd, uint32_t events);
    
    // 修改已注册的文件描述符事件
    void modFd(int fd, uint32_t events);
    
    // 移除监控的文件描述符
    void removeFd(int fd);
    
    // 等待事件发生
    int wait(int timeoutMs = -1);
    
    // 获取就绪事件数组
    const epoll_event* events() const { return readyEvents.data(); }

private:
    int epollFd = -1;                  // epoll实例文件描述符
    std::vector<epoll_event> readyEvents;  // 就绪事件数组
};