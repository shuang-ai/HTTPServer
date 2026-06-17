#include "../../include/http/HttpServer.h"

#include <any>
#include <functional>
#include <memory>

namespace http
{

/**
 * @brief 默认的HTTP请求回调函数
 * 
 * 当没有注册特定的请求处理函数时，此函数作为默认处理器。
 * 它总是返回404 Not Found状态，并指示关闭连接。
 * 
 * @param req HTTP请求对象（未使用）
 * @param resp HTTP响应对象指针，用于设置响应状态和消息
 */
void defaultHttpCallback(const HttpRequest &, HttpResponse *resp)
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

/**
 * @brief HttpServer构造函数
 * 
 * 初始化HTTP服务器实例，配置监听地址、底层TCP服务器以及SSL选项。
 * 同时绑定默认的请求处理逻辑到内部回调。
 * 
 * @param port 服务器监听的端口号
 * @param name 服务器名称，用于日志标识
 * @param useSSL 是否启用SSL/TLS加密支持
 * @param option TcpServer的配置选项，如线程池策略等
 */
HttpServer::HttpServer(int port,
                       const std::string &name,
                       bool useSSL,
                       muduo::net::TcpServer::Option option)
    : listenAddr_(port)
    , server_(&mainLoop_, listenAddr_, name, option)
    , useSSL_(useSSL)
    , httpCallback_(std::bind(&HttpServer::handleRequest, this, std::placeholders::_1, std::placeholders::_2))
{
    initialize();
}

/**
 * @brief 启动HTTP服务器
 * 
 * 记录启动日志，启动底层的TCP服务器并开始事件循环。
 * 此函数通常会阻塞当前线程，直到服务器停止。
 */
void HttpServer::start()
{
    LOG_WARN << "HttpServer[" << server_.name() << "] starts listening on" << server_.ipPort();
    server_.start();
    mainLoop_.loop();
}

/**
 * @brief 初始化服务器内部组件
 * 
 * 设置底层TCP服务器的连接回调和消息回调，
 * 将网络事件转发至HttpServer类的相应处理成员函数。
 */
void HttpServer::initialize()
{
    // 设置回调函数
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

/**
 * @brief 配置SSL上下文
 * 
 * 如果启用了SSL支持，则根据提供的配置创建并初始化SSL上下文。
 * 如果初始化失败，程序将记录错误并终止运行。
 * 
 * @param config SSL配置对象，包含证书、密钥等必要信息
 */
void HttpServer::setSslConfig(const ssl::SslConfig& config)
{
    if (useSSL_)
    {
        sslCtx_ = std::make_unique<ssl::SslContext>(config);
        if (!sslCtx_->initialize())
        {
            LOG_ERROR << "Failed to initialize SSL context";
            abort();
        }
    }
}

/**
 * @brief TCP连接状态变更回调
 * 
 * 处理新连接的建立或现有连接的断开。
 * 如果启用了SSL，则在连接建立时创建SSL包装对象并启动握手；
 * 在连接断开时清理对应的SSL资源。
 * 无论是否使用SSL，都会在连接上下文中初始化HttpContext对象。
 * 
 * @param conn TCP连接智能指针
 */
void HttpServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        if (useSSL_)
        {
            auto sslConn = std::make_unique<ssl::SslConnection>(conn, sslCtx_.get());
            sslConn->setMessageCallback(
                std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            sslConns_[conn] = std::move(sslConn);
            sslConns_[conn]->startHandshake();
        }
        conn->setContext(HttpContext());
    }
    else 
    {
        if (useSSL_)
        {
            sslConns_.erase(conn);
        }
    }
}

/**
 * @brief TCP消息接收回调
 * 
 * 处理从客户端接收到的数据。
 * 如果启用了SSL，先进行SSL解密处理，待握手完成后获取明文数据。
 * 随后使用HttpContext解析HTTP请求报文。
 * 如果解析成功且获取了完整的请求，则调用onRequest生成响应。
 * 解析错误或发生异常时，发送400 Bad Request并关闭连接。
 * 
 * @param conn TCP连接智能指针
 * @param buf 接收数据的缓冲区
 * @param receiveTime 数据接收的时间戳
 */
void HttpServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp receiveTime)
{
    try
    {
        // 这层判断只是代表是否支持ssl
        if (useSSL_)
        {
            LOG_INFO << "onMessage useSSL_ is true";
            // 1.查找对应的SSL连接
            auto it = sslConns_.find(conn);
            if (it != sslConns_.end())
            {
                LOG_INFO << "onMessage sslConns_ is not empty";
                // 2. SSL连接处理数据
                it->second->onRead(conn, buf, receiveTime);

                // 3. 如果 SSL 握手还未完成，直接返回
                if (!it->second->isHandshakeCompleted())
                {
                    LOG_INFO << "onMessage sslConns_ is not empty";
                    return;
                }

                // 4. 从SSL连接的解密缓冲区获取数据
                muduo::net::Buffer* decryptedBuf = it->second->getDecryptedBuffer();
                if (decryptedBuf->readableBytes() == 0)
                    return; // 没有解密后的数据

                // 5. 使用解密后的数据进行HTTP 处理
                buf = decryptedBuf; // 将 buf 指向解密后的数据
                LOG_INFO << "onMessage decryptedBuf is not empty";
            }
        }
        // HttpContext对象用于解析出buf中的请求报文，并把报文的关键信息封装到HttpRequest对象中
        HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parseRequest(buf, receiveTime)) // 解析一个http请求
        {
            // 如果解析http报文过程中出错
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }
        // 如果buf缓冲区中解析出一个完整的数据包才封装响应报文
        if (context->gotAll())
        {
            onRequest(conn, context->request());
            context->reset();
        }
    }
    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        LOG_ERROR << "Exception in onMessage: " << e.what();
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
}

/**
 * @brief 处理HTTP请求并生成响应
 * 
 * 根据解析出的HttpRequest对象，创建HttpResponse对象。
 * 检查Connection头部以决定是否保持长连接。
 * 调用用户注册的httpCallback_生成具体的响应内容。
 * 最后将响应序列化并发送回客户端，如果是短连接则关闭TCP连接。
 * 
 * @param conn TCP连接智能指针
 * @param req 解析完成的HTTP请求对象
 */
void HttpServer::onRequest(const muduo::net::TcpConnectionPtr &conn, const HttpRequest &req)
{
    const std::string &connection = req.getHeader("Connection");
    bool close = ((connection == "close") ||
                  (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
    HttpResponse response(close);

    // 根据请求报文信息来封装响应报文对象
    httpCallback_(req, &response); // 执行onHttpCallback函数

    // 可以给response设置一个成员，判断是否请求的是文件，如果是文件设置为true，并且存在文件位置在这里send出去。
    muduo::net::Buffer buf;
    response.appendToBuffer(&buf);
    // 打印完整的响应内容用于调试
    LOG_INFO << "Sending response:\n" << buf.toStringPiece().as_string();

    conn->send(&buf);
    // 如果是短连接的话，返回响应报文后就断开连接
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

/**
 * @brief 执行请求路由及中间件处理
 * 
 * 这是HTTP请求的核心处理逻辑。
 * 首先执行请求前中间件链。
 * 然后尝试通过路由器匹配URL路径并执行对应的处理函数。
 * 如果未找到路由，返回404错误。
 * 接着执行响应后中间件链。
 * 捕获中间件或路由处理中抛出的HttpResponse异常（用于提前结束请求，如CORS预检）。
 * 捕获其他标准异常并返回500内部服务器错误。
 * 
 * @param req HTTP请求对象
 * @param resp HTTP响应对象指针，用于写入处理结果
 */
void HttpServer::handleRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        // 处理请求前的中间件
        HttpRequest mutableReq = req;
        middlewareChain_.processBefore(mutableReq, resp);

        // 路由处理，true表示执行成功
        if (!router_.route(mutableReq, resp))
        {
            LOG_INFO << "请求的啥，url：" << req.method() << " " << req.path();
            LOG_INFO << "未找到路由，返回404";
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }

        // 处理响应后的中间件
        middlewareChain_.processAfter(*resp);
    }
    catch (const HttpResponse& res) 
    {
        // 处理中间件抛出的响应（如CORS预检请求）
        *resp = res;
    }
    catch (const std::exception& e) 
    {
        // 错误处理
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(e.what());
    }
}

} // namespace http