#include "http/HttpServer.h"
#include "utils/Logger.h"
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        // 设置日志级别
        Logger::instance().setLevel(DEBUG);
        
        // 解析命令行参数
        int port = 8080;
        int threads = 4;
        if (argc > 1) port = std::atoi(argv[1]);
        if (argc > 2) threads = std::atoi(argv[2]);

        LOG(INFO) << "Starting server on port " << port 
                 << " with " << threads << " worker threads";
        
        // 创建并启动服务器
        HttpServer server(port, threads);
        server.start();
    } catch (const std::exception& e) {
        LOG(FATAL) << "Server crashed: " << e.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}