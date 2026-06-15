#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <mutex>


#include "AiGame.h"
#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"


class LoginHandler;
class EntryHandler;
class RegisterHandler;
class MenuHandler;
class AiGameStartHandler;
class LogoutHandler;
class AiGameMoveHandler;
class GameBackendHandler;

#define DURING_GAME 1 
#define GAME_OVER 2

#define MAX_AIBOT_NUM 4096
/**
 * @brief 五子棋游戏服务器主类
 * 
 * 该类负责管理整个五子棋游戏的后端服务，包括HTTP服务器的初始化、
 * 会话管理、路由配置、中间件设置以及游戏逻辑的核心数据处理。
 * 它维护了在线用户状态、AI对局实例以及统计信息（如最高在线人数）。
 */
class GomokuServer
{
public:
    /**
     * @brief 构造函数
     * 
     * @param port 服务器监听的端口号
     * @param name 服务器名称标识
     * @param option Tcp服务器的选项配置，默认为不重用端口
     */
    GomokuServer(int port,
                 const std::string& name,
                 muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

    /**
     * @brief 设置工作线程数量
     * 
     * @param numThreads 线程池中的线程数量
     */             
    void setThreadNum(int numThreads);
    void start();
private:
    /**
     * @brief 初始化服务器组件
     * 
     * 协调调用会话、路由和中间件的初始化函数。
     */
    void initialize();

    /**
     * @brief 初始化会话管理器
     * 
     * 配置HTTP服务器的会话管理策略。
     */
    void initializeSession();

    /**
     * @brief 初始化路由表
     * 
     * 注册各个业务 handler 到对应的 URL 路径。
     */
    void initializeRouter();

    /**
     * @brief 配置中间件
     * 
     * 添加一系列中间件到HTTP服务器的处理链中。
     */
    void initializeMiddleware();
    
    /**
     * @brief 设置会话管理器
     * 
     * @param manager 会话管理器智能指针，所有权将转移给内部 httpServer_
     */
    void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
    {
        httpServer_.setSessionManager(std::move(manager));
    }

    /**
     * @brief 获取会话管理器指针
     * 
     * @return http::session::SessionManager* 当前使用的会话管理器指针
     */
    http::session::SessionManager*  getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }
    
    /**
     * @brief 处理重启人机对战游戏的请求
     * 
     * @param req HTTP请求对象，包含用户身份信息
     * @param resp HTTP响应对象，用于填充返回数据
     */
    void restartChessGameVsAi(const http::HttpRequest& req, http::HttpResponse* resp);

    /**
     * @brief 处理获取后端管理数据
     * 
     * @param req HTTP请求对象，包含用户身份信息
     * @param resp HTTP响应对象，用于填充返回数据
     */
    void getBackendData(const http::HttpRequest& req, http::HttpResponse* resp);

    /**
     * @brief 封装 HTTP 响应数据
     * 
     * @param version HTTP 版本
     * @param statusCode HTTP 状态码
     * @param statusMsg HTTP 状态信息
     * @param close 是否关闭连接
     * @param contentType 响应内容类型
     * @param contentLen 响应内容长度
     * @param body 响应内容
     * @param resp HTTP 响应对象
     */
    void packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
                     const std::string& statusMsg, bool close, const std::string& contentType,
                     int contentLen, const std::string& body, http::HttpResponse* resp);

    // 获取历史最高在线人数
    int getMaxOnline() const
    {
        return maxOnline_.load();
    }

    // 获取当前在线人数
    int getCurOnline() const
    {
        return onlineUsers_.size();
    }

    void updateMaxOnline(int online)
    {
        maxOnline_ = std::max(maxOnline_.load(), online);
    }

    // 获取用户总数
    int getUserCount()
    {
        std::string sql = "SELECT COUNT(*) as count FROM users";

        sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
        if (res->next())
        {
            return res->getInt("count");
        }
        return 0;
    }
    
private:
    friend class EntryHandler;
    friend class LoginHandler;
    friend class RegisterHandler;
    friend class MenuHandler;
    friend class AiGameStartHandler;
    friend class LogoutHandler;
    friend class AiGameMoveHandler;
    friend class GameBackendHandler;

private:
    enum GameType
    {
        NO_GAME = 0,
        MAN_VS_AI = 1,
        MAN_VS_MAN = 2
    };
    // 实际业务制定由GomokuServer来完成
    // 需要留意httpServer_提供哪些接口供使用
    http::HttpServer                                 httpServer_;
    http::MysqlUtil                                  mysqlUtil_;
    // userId -> AiBot
    std::unordered_map<int, std::shared_ptr<AiGame>> aiGames_;
    std::mutex                                       mutexForAiGames_;
    // userId -> 是否在游戏中
    std::unordered_map<int, bool>                    onlineUsers_;
    std::mutex                                       mutexForOnlineUsers_; 
    // 最高在线人数
    std::atomic<int>                                 maxOnline_;
};