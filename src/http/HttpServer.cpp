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
