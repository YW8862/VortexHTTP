#pragma once
#include <string>
#include <unordered_map>

/**
 * @brief HTTP请求解析器
 * 
 * 实现HTTP/1.1协议的请求行和头部解析
 * 支持状态机模式解析不完整请求
 */
class HttpParser {
public:
    // 解析状态枚举
    enum class State {
        METHOD,     // 解析请求方法
        PATH,       // 解析请求路径
        VERSION,    // 解析协议版本
        HEADER_KEY, // 解析头部字段名
        HEADER_VAL, // 解析头部字段值
        BODY        // 解析消息体（暂未实现）
    };

    /**
     * @brief 解析HTTP请求数据
     * @param data 输入数据指针
     * @param length 数据长度
     */
    void parse(const char* data, size_t length);
    
    // 状态检查方法
    bool isComplete() const { return parseComplete; }
    
    // 获取解析结果
    const std::string& getMethod() const { return method; }
    const std::string& getPath() const { return path; }
    const std::string& getVersion() const { return version; }
    std::string getHeader(const std::string& key) const;

private:
    State state = State::METHOD;  // 当前解析状态
    bool parseComplete = false;   // 解析完成标志
    
    // 解析结果存储
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string currentHeaderKey; // 临时存储当前处理的header key
};