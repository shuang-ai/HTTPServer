#pragma once

#include <muduo/net/TcpServer.h>

namespace http
{

/**
 * @brief HTTP 响应类，用于封装 HTTP 响应的状态行、头部信息和主体内容。
 */
class HttpResponse 
{
public:
    /**
     * @brief HTTP 状态码枚举。
     */
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k500InternalServerError = 500,
    };

    /**
     * @brief 构造函数。
     * 
     * @param close 是否关闭连接，默认为 true。
     */
    HttpResponse(bool close = true)
        : statusCode_(kUnknown)
        , closeConnection_(close)
    {}

    /**
     * @brief 设置 HTTP 版本。
     * 
     * @param version HTTP 版本字符串，例如 "HTTP/1.1"。
     */
    void setVersion(std::string version)
    { httpVersion_ = version; }

    /**
     * @brief 设置 HTTP 状态码。
     * 
     * @param code HTTP 状态码。
     */
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    /**
     * @brief 获取 HTTP 状态码。
     * 
     * @return HttpStatusCode 当前设置的状态码。
     */
    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    /**
     * @brief 设置状态消息。
     * 
     * @param message 状态消息字符串，例如 "OK" 或 "Not Found"。
     */
    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    /**
     * @brief 设置是否在响应后关闭连接。
     * 
     * @param on 如果为 true，则响应后关闭连接；否则保持连接。
     */
    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    /**
     * @brief 检查是否需要在响应后关闭连接。
     * 
     * @return bool 如果需要关闭连接返回 true，否则返回 false。
     */
    bool closeConnection() const
    { return closeConnection_; }
    
    /**
     * @brief 设置 Content-Type 头部。
     * 
     * @param contentType 内容类型字符串，例如 "text/html" 或 "application/json"。
     */
    void setContentType(const std::string& contentType)
    { addHeader("Content-Type", contentType); }

    /**
     * @brief 设置 Content-Length 头部。
     * 
     * @param length 响应主体的长度。
     */
    void setContentLength(uint64_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    /**
     * @brief 添加自定义 HTTP 头部字段。
     * 
     * @param key 头部字段的名称。
     * @param value 头部字段的值。
     */
    void addHeader(const std::string& key, const std::string& value)
    { headers_[key] = value; }
    
    /**
     * @brief 设置响应主体内容。
     * 
     * @param body 响应主体的字符串数据。
     */
    void setBody(const std::string& body)
    { 
        body_ = body;
        // body_ += "\0";
    }

    /**
     * @brief 设置完整的状态行信息（版本、状态码、状态消息）。
     * 
     * @param version HTTP 版本字符串。
     * @param statusCode HTTP 状态码。
     * @param statusMessage 状态消息字符串。
     */
    void setStatusLine(const std::string& version,
                         HttpStatusCode statusCode,
                         const std::string& statusMessage);

    /**
     * @brief 设置错误头部（当前为空实现）。
     */
    void setErrorHeader(){}

    /**
     * @brief 将 HTTP 响应序列化并追加到缓冲区中。
     * 
     * @param outputBuf 目标缓冲区指针，用于存储序列化后的响应数据。
     */
    void appendToBuffer(muduo::net::Buffer* outputBuf) const;
private:
    std::string                        httpVersion_; 
    HttpStatusCode                     statusCode_;
    std::string                        statusMessage_;
    bool                               closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string                        body_;
    bool                               isFile_;
};

} // namespace http