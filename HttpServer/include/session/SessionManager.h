#pragma once

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <memory>
#include <random>

namespace http
{
namespace session
{

/**
 * @brief 会话管理器，负责会话的创建、获取、销毁及清理工作。
 */
class SessionManager
{
public:
    /**
     * @brief 构造函数，初始化会话管理器。
     * @param storage 会话存储接口的唯一指针，用于持久化或缓存会话数据。
     */
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    /**
     * @brief 从请求中获取现有会话，若不存在则创建新会话。
     * 
     * 该函数会检查请求中的 Cookie 是否包含有效的会话 ID。
     * 如果存在且有效，则从存储中加载会话；否则生成新的会话 ID，
     * 创建新会话，并将其 ID 设置到响应 Cookie 中。
     * 
     * @param req HTTP 请求对象，用于提取 Cookie 中的会话 ID。
     * @param resp HTTP 响应对象指针，用于在创建新会话时设置 Set-Cookie 头。
     * @return std::shared_ptr<Session> 返回指向会话对象的共享指针。
     */
    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);
    
    /**
     * @brief 销毁指定 ID 的会话。
     * 
     * 从底层存储中删除会话数据，使该会话 ID 失效。
     * 
     * @param sessionId 需要销毁的会话 ID。
     */
    void destroySession(const std::string& sessionId);

    /**
     * @brief 清理所有过期的会话。
     * 
     * 遍历存储中的会话，移除超过存活时间的会话记录，以释放资源。
     */
    void cleanExpiredSessions();

    /**
     * @brief 更新会话数据。
     * 
     * 将会话对象的当前状态保存到底层存储中。
     * 
     * @param session 需要更新的会话对象的共享指针。
     */
    void updateSession(std::shared_ptr<Session> session)
    {
        storage_->save(session);
    }
private:
    /**
     * @brief 生成唯一的随机会话 ID。
     * @return std::string 生成的会话 ID 字符串。
     */
    std::string generateSessionId();

    /**
     * @brief 从 HTTP 请求的 Cookie 中提取会话 ID。
     * @param req HTTP 请求对象。
     * @return std::string 提取到的会话 ID，若不存在则返回空字符串。
     */
    std::string getSessionIdFromCookie(const HttpRequest& req);

    /**
     * @brief 将会话 ID 设置到 HTTP 响应的 Cookie 中。
     * @param sessionId 需要设置的会话 ID。
     * @param resp HTTP 响应对象指针，用于添加 Set-Cookie 头。
     */
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);

private:
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_; // 用于生成随机会话id
};

} // namespace session
} // namespace http