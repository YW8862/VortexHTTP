#pragma once
#include "core/Epoll.h"
#include "core/ThreadPool.h"
#include "http/HttpParser.h"
#include "utils/Logger.h"
#include <netinet/in.h>

/**
 * @brief HTTP服务器主类
 * 
 * 整合Epoll事件循环和线程池，实现高并发服务
 */
class HttpServer 
{
public:
    // 构造函数指定端口和线程数量
    HttpServer(int port, int threadNum);
    
    // 启动服务器主循环
    void start();

private:
    // 处理Epoll事件
    void handleEvent(int fd, uint32_t events);
    
    // 接受新的客户端连接
    void acceptConnection();
    
    // 关闭指定连接
    void closeConnection(int fd);
    
    // 处理HTTP请求
    void handleRequest(int fd);
    
    // 发送HTTP响应
    void sendResponse(int fd);

    //发送错误响应
    void sendErrorResponse(int fd, int code, const std::string& message);

    int port;                // 服务器监听端口
    int listenFd;            // 监听socket的文件描述符
    Epoll epoll;             // Epoll事件管理器
    ThreadPool pool;         // 线程池
    struct sockaddr_in addr; // 服务器地址结构
};