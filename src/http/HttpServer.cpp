#include "http/HttpServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

/**
 * @brief 构造函数初始化服务器
 * @param port 监听端口号
 * @param threadNum 线程池工作线程数量
 */
HttpServer::HttpServer(int port, int threadNum) 
    : port(port), pool(threadNum)
{
    // 创建监听socket（非阻塞模式）
    listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenFd == -1) {
        LOG(FATAL) << "Socket creation failed: " << strerror(errno);
        exit(EXIT_FAILURE);
    }

    // 设置地址重用选项
    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG(ERROR) << "Set SO_REUSEADDR failed: " << strerror(errno);
    }

    // 绑定地址结构体
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // 绑定socket
    if (bind(listenFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG(FATAL) << "Bind failed: " << strerror(errno);
        close(listenFd);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(listenFd, SOMAXCONN) < 0) {
        LOG(FATAL) << "Listen failed: " << strerror(errno);
        close(listenFd);
        exit(EXIT_FAILURE);
    }

    // 将监听socket加入epoll
    epoll.addFd(listenFd, EPOLLIN);
    LOG(INFO) << "Server initialized on port " << port;
}

/**
 * @brief 启动服务器主循环
 */
void HttpServer::start() {
    LOG(INFO) << "Server started, entering event loop";
    while (true) {
        // 等待事件发生（无限阻塞）
        int numEvents = epoll.wait();
        
        // 处理所有就绪事件
        for (int i = 0; i < numEvents; ++i) {
            const auto& event = epoll.events()[i];
            int fd = event.data.fd;
            uint32_t events = event.events;

            if (fd == listenFd) {
                // 处理新连接请求
                acceptConnection();
            } else {
                // 处理客户端连接事件
                if (events & (EPOLLERR | EPOLLHUP)) {
                    LOG(WARNING) << "Error event on fd " << fd;
                    closeConnection(fd);
                } else if (events & EPOLLIN) {
                    // 将读事件提交给线程池处理
                    pool.enqueue([this, fd] {
                        handleRequest(fd);
                    });
                }
            }
        }
    }
}

/**
 * @brief 发送错误响应
 */
void HttpServer::sendErrorResponse(int fd, int code, const std::string& message) {
    const std::string body = 
        "<html><body><h1>" + std::to_string(code) + " " + message + "</h1></body></html>";
    
    const std::string response = 
        "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    if (send(fd, response.data(), response.size(), 0) == -1) {
        LOG(ERROR) << "Failed to send error response to fd " << fd;
    }
}


/**
 * @brief 接受新的客户端连接
 * 
 * 1. 使用accept4非阻塞接收新连接
 * 2. 设置socket为非阻塞模式
 * 3. 将新连接加入epoll监控
 */
void HttpServer::acceptConnection() {
    struct sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    
    // 使用accept4非阻塞接收连接（SOCK_NONBLOCK）
    int connFd = accept4(listenFd, (struct sockaddr*)&clientAddr, 
                       &addrLen, SOCK_NONBLOCK);
    if (connFd == -1) {
        LOG(ERROR) << "Accept failed: " << strerror(errno);
        return;
    }

    // 转换客户端地址为字符串
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
    LOG(INFO) << "Accepted connection from " << ipStr << ":" 
             << ntohs(clientAddr.sin_port) << " (fd: " << connFd << ")";

    try {
        // 将新连接加入epoll（边缘触发模式）
        epoll.addFd(connFd, EPOLLIN | EPOLLET);
        LOG(DEBUG) << "Added new connection to epoll (fd: " << connFd << ")";
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to add fd " << connFd << " to epoll: " << e.what();
        close(connFd);
    }
}


// 新增发送函数
void sendAll(int fd, const char* buf, size_t len) {
    size_t totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = send(fd, buf + totalSent, len - totalSent, 0);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 等待可写事件（需配合EPOLLOUT事件处理）
                continue;
            }
            throw std::runtime_error("send error: " + std::string(strerror(errno)));
        }
        totalSent += sent;
    }
}


/**
 * @brief 处理HTTP请求
 * 
 * 1. 读取请求数据
 * 2. 解析HTTP请求
 * 3. 生成并发送响应
 * 4. 异常处理和资源清理
 */
void HttpServer::handleRequest(int fd) {
    char buffer[4096];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        // 移除终止符
        std::string request(buffer, bytesRead);
        
        HttpParser parser;
        parser.parse(request.data(), request.size());

        // 构造动态响应
        const std::string body = "Hello World";
        const std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + 
            body;

        // 完整发送数据
        try {
            sendAll(fd, response.data(), response.size());
            LOG(INFO) << "Sent " << response.size() << " bytes to fd " << fd;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Send failed: " << e.what();
        }
    }
    
    closeConnection(fd);
}

/**
 * @brief 关闭连接并清理资源
 * 
 * 1. 从epoll移除监控
 * 2. 关闭socket文件描述符
 * 3. 更新连接计数
 */
void HttpServer::closeConnection(int fd) {
    try {
        // 从epoll移除
        epoll.removeFd(fd);
        LOG(DEBUG) << "Removed fd " << fd << " from epoll";
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to remove fd " << fd << ": " << e.what();
    }

    // 关闭socket
    if (close(fd) == -1) {
        LOG(ERROR) << "Close failed for fd " << fd << ": " << strerror(errno);
    } else {
        LOG(INFO) << "Closed connection (fd: " << fd << ")";
    }
}

