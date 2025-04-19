#include "http/HttpParser.h"
#include "utils/Logger.h"
#include <algorithm>

void HttpParser::parse(const char* data, size_t length) {
    const char* end = data + length;
    
    while (data < end && !parseComplete) {
        switch (state) {
            case State::METHOD:
                // 解析方法直到遇到空格
                if (const char* space = std::find(data, end, ' '); space != end) {
                    method.assign(data, space);
                    data = space + 1;
                    state = State::PATH;
                }
                break;
                
            case State::PATH:
                // 解析路径直到遇到空格
                if (const char* space = std::find(data, end, ' '); space != end) {
                    path.assign(data, space);
                    data = space + 1;
                    state = State::VERSION;
                }
                break;
                
            case State::VERSION:
                // 查找CRLF结束符
                if (const char* crlf = std::find(data, end, '\r'); crlf != end && crlf+1 < end) {
                    version.assign(data, crlf);
                    data = crlf + 2; // 跳过\r\n
                    state = State::HEADER_KEY;
                }
                break;
                
            case State::HEADER_KEY: {
                // 查找冒号分隔符
                const char* colon = std::find(data, end, ':');
                if (colon == end) {
                    // 空行表示头部结束
                    if (data + 1 < end && *data == '\r' && *(data+1) == '\n') {
                        parseComplete = true;
                        data += 2;
                    }
                    break;
                }
                
                currentHeaderKey.assign(data, colon);
                data = colon + 1;
                state = State::HEADER_VAL;
                break;
            }
                
            case State::HEADER_VAL:
                // 查找CRLF结束符
                if (const char* crlf = std::find(data, end, '\r'); crlf != end && crlf+1 < end) {
                    // 跳过首部空白字符
                    while (data < crlf && (*data == ' ' || *data == '\t')) ++data;
                    
                    std::string value(data, crlf);
                    headers[currentHeaderKey] = value;
                    data = crlf + 2; // 跳过\r\n
                    state = State::HEADER_KEY;
                }
                break;
                
            case State::BODY:
                // 预留body处理逻辑
                parseComplete = true;
                break;
        }
    }
    
    LOG(DEBUG) << "Parsed request: " << method << " " << path << " " << version;
}

std::string HttpParser::getHeader(const std::string& key) const {
    auto it = headers.find(key);
    return it != headers.end() ? it->second : "";
}