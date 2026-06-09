#pragma once 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../router/Router.h"
#include "../session/SessionManager.h"
#include "../middleware/MiddlewareChain.h"
#include "../middleware/cors/CorsMiddleware.h"
#include "../ssl/SslConnection.h"
#include "../ssl/SslContext.h"

class HttpRequest;
class HttpResponse;

namespace http
{

/**
 * @brief HTTP 服务器类，基于 Muduo 网络库构建。
 * 
 * 支持 HTTP/HTTPS 协议，提供路由注册、中间件链、会话管理等功能。
 * 该类不可拷贝。
 */
class HttpServer : muduo::noncopyable
{
public:
    /**
     * @brief HTTP 请求回调函数类型定义。
     * 
     * @param req 只读的 HTTP 请求对象引用。
     * @param resp 可写的 HTTP 响应对象指针，用于设置响应内容。
     */
    using HttpCallback = std::function<void (const http::HttpRequest&, http::HttpResponse*)>;

    /**
     * @brief 构造 HTTP 服务器实例。
     * 
     * @param port 服务器监听的端口号。
     * @param name 服务器名称，用于日志标识。
     * @param useSSL 是否启用 SSL/TLS 加密连接，默认为 false。
     * @param option TcpServer 的选项配置，默认为 kNoReusePort。
     */
    HttpServer(int port,
               const std::string& name,
               bool useSSL = false,
               muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    
    /**
     * @brief 设置工作线程数量。
     * 
     * @param numThreads 工作线程的数量。
     */
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    /**
     * @brief 启动 HTTP 服务器，开始监听连接。
     */
    void start();

    /**
     * @brief 获取主事件循环指针。
     * 
     * @return muduo::net::EventLoop* 主事件循环指针。
     */
    muduo::net::EventLoop* getLoop() const 
    { 
        return server_.getLoop(); 
    }

    /**
     * @brief 设置默认的 HTTP 请求回调函数。
     * 
     * 当路由未匹配时，可能会调用此默认回调（具体行为取决于内部实现逻辑）。
     * 
     * @param cb 默认的 HTTP 请求处理回调函数。
     */
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    /**
     * @brief 注册 GET 请求的路由处理器（简单接口）。
     * 
     * @param path 请求路径。
     * @param cb 处理该路径 GET 请求的回调函数。
     */
    void Get(const std::string& path, const HttpCallback& cb)
    {
        router_.registerCallback(HttpRequest::kGet, path, cb);
    }
    
    /**
     * @brief 注册 GET 请求的路由处理器（复杂业务对象）。
     * 
     * @param path 请求路径。
     * @param handler 处理该路径 GET 请求的业务处理器指针。
     */
    void Get(const std::string& path, router::Router::HandlerPtr handler)
    {
        router_.registerHandler(HttpRequest::kGet, path, handler);
    }

    /**
     * @brief 注册 POST 请求的路由处理器（简单接口）。
     * 
     * @param path 请求路径。
     * @param cb 处理该路径 POST 请求的回调函数。
     */
    void Post(const std::string& path, const HttpCallback& cb)
    {
        router_.registerCallback(HttpRequest::kPost, path, cb);
    }

    /**
     * @brief 注册 POST 请求的路由处理器（复杂业务对象）。
     * 
     * @param path 请求路径。
     * @param handler 处理该路径 POST 请求的业务处理器指针。
     */
    void Post(const std::string& path, router::Router::HandlerPtr handler)
    {
        router_.registerHandler(HttpRequest::kPost, path, handler);
    }

    /**
     * @brief 注册动态路由处理器（支持正则表达式等动态匹配）。
     * 
     * @param method HTTP 请求方法（如 GET, POST 等）。
     * @param path 动态路径模式。
     * @param handler 处理该动态路径的业务处理器指针。
     */
    void addRoute(HttpRequest::Method method, const std::string& path, router::Router::HandlerPtr handler)
    {
        router_.addRegexHandler(method, path, handler);
    }

    /**
     * @brief 注册动态路由处理函数（支持正则表达式等动态匹配）。
     * 
     * @param method HTTP 请求方法（如 GET, POST 等）。
     * @param path 动态路径模式。
     * @param callback 处理该动态路径的回调函数。
     */
    void addRoute(HttpRequest::Method method, const std::string& path, const router::Router::HandlerCallback& callback)
    {
        router_.addRegexCallback(method, path, callback);
    }

    /**
     * @brief 设置会话管理器。
     * 
     * @param manager 会话管理器的唯一指针，所有权转移至本服务器。
     */
    void setSessionManager(std::unique_ptr<session::SessionManager> manager)
    {
        sessionManager_ = std::move(manager);
    }

    /**
     * @brief 获取会话管理器指针。
     * 
     * @return session::SessionManager* 会话管理器指针，若未设置则返回 nullptr。
     */
    session::SessionManager* getSessionManager() const
    {
        return sessionManager_.get();
    }

    /**
     * @brief 添加中间件到中间件链。
     * 
     * @param middleware 中间件对象的共享指针。
     */
    void addMiddleware(std::shared_ptr<middleware::Middleware> middleware) 
    {
        middlewareChain_.addMiddleware(middleware);
    }

    /**
     * @brief 启用或禁用 SSL 功能。
     * 
     * @param enable true 表示启用 SSL，false 表示禁用。
     */
    void enableSSL(bool enable) 
    {
        useSSL_ = enable;
    }

    /**
     * @brief 配置 HTTPS (SSL/TLS) 连接所需的证书和密钥信息。
     * 
     * @param config SSL 配置结构体，包含证书路径、密钥路径等信息。
     */
    void setSslConfig(const ssl::SslConfig& config);

private:
    /**
     * @brief 初始化服务器内部组件。
     */
    void initialize();

    /**
     * @brief TCP 连接建立或断开时的回调处理。
     * 
     * @param conn TCP 连接指针。
     */
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    /**
     * @brief TCP 连接收到数据时的回调处理。
     * 
     * @param conn TCP 连接指针。
     * @param buf 接收到的数据缓冲区。
     * @param receiveTime 数据接收时间戳。
     */
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp receiveTime);

    /**
     * @brief 处理解析完成的 HTTP 请求。
     * 
     * @param conn TCP 连接指针。
     * @param req 解析后的 HTTP 请求对象。
     */
    void onRequest(const muduo::net::TcpConnectionPtr&, const HttpRequest&);

    /**
     * @brief 核心请求处理逻辑，包括中间件执行、路由匹配等。
     * 
     * @param req HTTP 请求对象。
     * @param resp HTTP 响应对象指针，用于写入响应数据。
     */
    void handleRequest(const HttpRequest& req, HttpResponse* resp);
    
private:
    muduo::net::InetAddress                      listenAddr_; // 监听地址
    muduo::net::TcpServer                        server_; 
    muduo::net::EventLoop                        mainLoop_; // 主循环
    HttpCallback                                 httpCallback_; // 回调函数
    router::Router                               router_; // 路由
    std::unique_ptr<session::SessionManager>     sessionManager_; // 会话管理器
    middleware::MiddlewareChain                  middlewareChain_; // 中间件链
    std::unique_ptr<ssl::SslContext>             sslCtx_; // SSL 上下文
    bool                                         useSSL_; // 是否使用 SSL   
    // TcpConnectionPtr -> SslConnectionPtr 
    std::map<muduo::net::TcpConnectionPtr, std::unique_ptr<ssl::SslConnection>> sslConns_;
}; 

} // namespace http