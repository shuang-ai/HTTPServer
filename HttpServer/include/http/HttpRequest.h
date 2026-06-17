#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include <muduo/base/Timestamp.h>

namespace http
{

namespace session
{
class Session;
}

class HttpRequest
{
public:
    enum Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions
    };
    
    HttpRequest()
        : method_(kInvalid)
        , version_("Unknown")
    {
    }
    
    // 设置HTTP请求的接收时间戳
    void setReceiveTime(muduo::Timestamp t);
    muduo::Timestamp receiveTime() const { return receiveTime_; }
    // 根据给定的字符串范围设置 HTTP 请求方法。
    bool setMethod(const char* start, const char* end);
    Method method() const { return method_; }
    // 设置请求路径
    void setPath(const char* start, const char* end);
    std::string path() const { return path_; }
    // 设置HTTP请求的路径参数
    void setPathParameters(const std::string &key, const std::string &value);
/**
 * @brief 获取指定键名的路径参数值
 * 
 * @param key 路径参数的键名
 * @return std::string 如果找到对应的路径参数则返回其值，否则返回空字符串
 */
    std::string getPathParameters(const std::string &key) const;

    // 解析 URL 中问号 ? 后面的查询字符串，并将提取出的键值对（Key-Value）
    // 存储到成员变量 queryParameters_ 中。
    // 解析并设置 HTTP 请求的查询参数
    void setQueryParameters(const char* start, const char* end);
    // 获取指定键名的查询参数值
    std::string getQueryParameters(const std::string &key) const;
    
    void setVersion(std::string v)
    {
        version_ = v;
    }

    std::string getVersion() const
    {
        return version_;
    }
    
    // 解析并添加一个 HTTP 请求头（Header）到 headers_ 成员变量中。
    void addHeader(const char* start, const char* colon, const char* end);
    // 获取HTTP请求中指定字段的头部值
    std::string getHeader(const std::string& field) const;

    const std::map<std::string, std::string>& headers() const
    { return headers_; }

    // 设置请求体
    void setBody(const std::string& body) { content_ = body; }
    void setBody(const char* start, const char* end) 
    { 
        if (end >= start) 
        {
            content_.assign(start, end - start); 
        }
    }
    
    std::string getBody() const
    { return content_; }

    void setContentLength(uint64_t length)
    { contentLength_ = length; }
    
    uint64_t contentLength() const
    { return contentLength_; }

    void setSession(std::shared_ptr<session::Session> session)
    { session_ = std::move(session); }

    std::shared_ptr<session::Session> getSession() const
    { return session_; }

    bool hasSession() const
    { return static_cast<bool>(session_); }

    // 交换当前 HttpRequest 对象与另一个 HttpRequest 对象的内容。
    void swap(HttpRequest& that);

private:
    Method                                       method_; // 请求方法
    std::string                                  version_; // http版本
    std::string                                  path_; // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_; // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    muduo::Timestamp                             receiveTime_; // 接收时间
    std::map<std::string, std::string>           headers_; // 请求头
    std::string                                  content_; // 请求体
    uint64_t                                     contentLength_ { 0 }; // 请求体长度
    std::shared_ptr<session::Session>            session_;
};

} // namespace http