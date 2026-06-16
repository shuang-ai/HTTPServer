#include"../include/session/SessionManager.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace http
{
namespace session
{

// 初始化会话管理器，设置会话存储对象和随机数生成器
SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
    : storage_(std::move(storage)) 
    , rng_(std::random_device{}()) // 初始化随机数生成器，用于生成随机的会话ID
{}

/**
 * @brief 获取或创建会话对象
 * 
 * 该函数尝试从请求的Cookie中提取会话ID并加载对应的会话。
 * 如果会话不存在、已过期或Cookie中未包含有效的会话ID，则创建一个新的会话，
 * 并将新的会话ID设置到响应Cookie中。对于现有的有效会话，会更新其管理器引用并刷新状态。
 * 最后，将会话数据持久化存储并返回会话对象。
 *
 * @param req HTTP请求对象，用于从中提取Cookie以获取会话ID
 * @param resp HTTP响应对象指针，用于在创建新会话时设置Set-Cookie头
 * @return std::shared_ptr<Session> 返回有效的会话对象共享指针，可能是新创建的或从存储中加载的
 */
std::shared_ptr<Session> SessionManager::getSession(const HttpRequest& req, HttpResponse* resp)
{   
    // 从 HTTP 请求的 Cookie 头中提取 sessionId 字符串
    // 如果 Cookie 中没有 sessionId，则返回空字符串
    std::string sessionId = getSessionIdFromCookie(req);
    
    std::shared_ptr<Session> session;

    // 如果请求中包含会话ID，则尝试加载会话
    if (!sessionId.empty())
    {
        session = storage_->load(sessionId);
    }

    // 4. 判断会话是否有效：
    // 条件 A: session 为空指针（说明 Cookie 里的 ID 无效或存储中不存在）
    // 条件 B: session 存在但已过期（isExpired() 返回 true）
    if (!session || session->isExpired())
    {
        sessionId = generateSessionId();
        session = std::make_shared<Session>(sessionId, this);
        setSessionCookie(sessionId, resp);
    }
    else 
    {
        // 如果会话有效且未过期，更新会话的管理器引用
        // 这通常用于确保 Session 对象持有正确的 Manager 指针（可能在反序列化后丢失）
        session->setManager(this); // 为现有会话设置管理器
    }

    // 防止活跃用户因为长时间操作而被判定为过期
    session->refresh();
    // 将会话状态持久化保存
    storage_->save(session);  // 这里可能有问题，需要确保正确保存会话
    // 返回有效的会话对象给调用者
    return session;
}

// 生成唯一的会话标识符，确保会话的唯一性和安全性
std::string SessionManager::generateSessionId()
{
    std::stringstream ss;
    std::uniform_int_distribution<> dist(0, 15);

    // 生成32个字符的会话ID，每个字符是一个十六进制数字
    for (int i = 0; i < 32; ++i)
    {
        ss << std::hex << dist(rng_);
    }
    return ss.str();
}

void SessionManager::destroySession(const std::string& sessionId)
{
    storage_->remove(sessionId);
}

void SessionManager::cleanExpiredSessions()
{
    // 注意：这个实现依赖于具体的存储实现
    // 对于内存存储，可以在加载时检查是否过期
    // 对于其他存储的实现，可能需要定期清理过期会话
}

std::string SessionManager::getSessionIdFromCookie(const HttpRequest& req)
{
    std::string sessionId;
    std::string cookie = req.getHeader("Cookie");

    if (!cookie.empty())
    {
        size_t pos = cookie.find("sessionId=");
        if (pos != std::string::npos)
        {
            pos += 10; // 跳过"sessionId="
            size_t end = cookie.find(';', pos);
            if (end != std::string::npos)
            {
                sessionId = cookie.substr(pos, end - pos);
            }
            else
            {
                sessionId = cookie.substr(pos);
            }
        }
    }
    
    return sessionId;
}

void SessionManager::setSessionCookie(const std::string& sessionId, HttpResponse* resp)
{
    // 设置会话ID到响应头中，作为Cookie
    std::string cookie = "sessionId=" + sessionId + "; Path=/; HttpOnly";
    resp->addHeader("Set-Cookie", cookie);
}

} // namespace session
} // namespace http