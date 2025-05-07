#include "core/Epoll.h"
#include "utils/Logger.h"
#include <stdexcept>
#include <unistd.h>
#include <cstring>

// 构造函数创建epoll实例
Epoll::Epoll() : readyEvents(MAX_EVENTS) 
{
    // 创建epoll实例，size参数在现代Linux中已忽略，但需>0
    epollFd = epoll_create1(0);
    if (epollFd == -1) 
    {
        LOG(FATAL) << "epoll_create1 failed: " << strerror(errno);
        throw std::runtime_error("epoll_create1 failed");
    }
    LOG(INFO) << "Epoll instance created (fd: " << epollFd << ")";
}

// 析构函数关闭epoll文件描述符
Epoll::~Epoll() 
{
    if (epollFd >= 0) 
    {
        close(epollFd);
        LOG(DEBUG) << "Epoll instance (fd: " << epollFd << ") destroyed";
    }
}

// 向epoll实例添加文件描述符
void Epoll::addFd(int fd, uint32_t events) 
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) 
    {
        LOG(ERROR) << "Failed to add fd " << fd << ": " << strerror(errno);
        throw std::runtime_error("epoll_ctl add failed");
    }
    LOG(DEBUG) << "Added fd " << fd << " to epoll with events 0x" 
              << std::hex << events;
}

// 修改已注册的文件描述符事件
void Epoll::modFd(int fd, uint32_t events) 
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == -1) 
    {
        LOG(ERROR) << "Failed to modify fd " << fd << ": " << strerror(errno);
        throw std::runtime_error("epoll_ctl mod failed");
    }
    LOG(DEBUG) << "Modified fd " << fd << " events to 0x" << std::hex << events;
}

// 从epoll移除文件描述符
void Epoll::removeFd(int fd) 
{
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr) == -1) 
    {
        LOG(ERROR) << "Failed to remove fd " << fd << ": " << strerror(errno);
        throw std::runtime_error("epoll_ctl del failed");
    }
    LOG(DEBUG) << "Removed fd " << fd << " from epoll";
}

// 等待事件发生，返回就绪事件数量
int Epoll::wait(int timeoutMs) 
{
    int numEvents = epoll_wait(epollFd, readyEvents.data(), 
                             static_cast<int>(readyEvents.size()), timeoutMs);
    if (numEvents == -1) 
    {
        // 忽略信号中断的情况
        if (errno != EINTR) 
        {
            LOG(ERROR) << "epoll_wait failed: " << strerror(errno);
            throw std::runtime_error("epoll_wait failed");
        }
        return 0;
    }
    LOG(DEBUG) << "Epoll wait returned " << numEvents << " events";
    return numEvents;
}