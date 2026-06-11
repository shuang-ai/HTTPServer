#pragma once

#include <iostream>

#include <muduo/net/TcpServer.h>

#include "HttpRequest.h"

namespace http
{

class HttpContext 
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行
        kExpectHeaders, // 解析请求头
        kExpectBody, // 解析请求体
        kGotAll, // 解析完成
    };
    
    HttpContext()
    : state_(kExpectRequestLine)
    {}
    // 解析HTTP请求报文
    bool parseRequest(muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

    /**
     * @brief 检查当前状态是否为“已获取全部”状态。
     *
     * @return true 如果当前状态等于 kGotAll。
     * @return false 如果当前状态不等于 kGotAll。
     */
    bool gotAll() const 
    { return state_ == kGotAll;  }

    /**
     * @brief 重置解析器状态并清空当前请求数据。
     *
     * 将内部状态机重置为初始状态（期待请求行），并通过交换技巧
     * 高效地清空 request_ 对象中积累的 HTTP 请求数据，以便复用内存。
     */
    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }

    /**
     * @brief 获取当前请求对象。
     *
     * @return const HttpRequest& 当前请求对象。
     */
    const HttpRequest& request() const
    { return request_;}

    /**
     * @brief 获取 HTTP 请求对象的引用。
     *
     * @return HttpRequest& 返回内部存储的 HTTP 请求对象的可修改引用。
     */
    HttpRequest& request()
    { return request_;}

private:
    // 解析 HTTP 请求行（Request Line）
    bool processRequestLine(const char* begin, const char* end);
private:
    HttpRequestParseState state_;
    HttpRequest           request_;
};

} // namespace http