#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace http
{

namespace session
{

class SessionManager;

/**
 * @brief 表示一个HTTP会话对象，用于管理用户会话状态和数据。
 * 
 * 该类继承自 std::enable_shared_from_this 以支持安全的共享指针管理。
 * 会话包含唯一的ID、过期时间、关联的管理器以及键值对数据存储空间。
 */
class Session : public std::enable_shared_from_this<Session>
{
public:
    /**
     * @brief 构造一个新的会话对象。
     * 
     * @param sessionId 会话的唯一标识符。
     * @param sessionManager 指向管理此会话的 SessionManager 实例的指针。
     * @param maxAge 会话的最大存活时间（秒），默认值为3600秒（1小时）。
     */
    Session(const std::string& sessionId, SessionManager* sessionManager, int maxAge = 3600); // 默认1小时过期
    
    /**
     * @brief 获取会话ID。
     * 
     * @return const std::string& 返回会话ID的常量引用。
     */
    const std::string& getId() const 
    { return sessionId_; }

    /**
     * @brief 检查会话是否已过期。
     * 
     * @return bool 如果当前时间超过会话的过期时间则返回 true，否则返回 false。
     */
    bool isExpired() const;

    /**
     * @brief 刷新会话的过期时间。
     * 
     * 根据创建时设定的 maxAge 重新计算并更新 expiryTime_。
     */
    void refresh(); // 刷新过期时间

    /**
     * @brief 设置管理此会话的 SessionManager。
     * 
     * @param sessionManager 指向新的 SessionManager 实例的指针。
     */
    void setManager(SessionManager* sessionManager) 
    { sessionManager_ = sessionManager; }

    /**
     * @brief 获取管理此会话的 SessionManager。
     * 
     * @return SessionManager* 返回指向 SessionManager 的指针。
     */
    SessionManager* getManager() const 
    { return sessionManager_; }

    // 数据存取
    void setValue(const std::string&key, const std::string&value);

    /**
     * @brief 获取会话中的数据项。
     * 
     * @param key 数据项的键。
     * @return std::string 返回对应键的值，如果键不存在则返回空字符串。
     */
    std::string getValue(const std::string&key) const;

    /**
     * @brief 从会话中移除指定键的数据项。
     * 
     * @param key 要移除的数据项的键。
     */
    void remove(const std::string&key);
    
    /**
     * @brief 清除会话中的所有数据项。
     */
    void clear();
private:
    std::string                                  sessionId_;
    std::unordered_map<std::string, std::string> data_;
    std::chrono::system_clock::time_point        expiryTime_;
    int                                          maxAge_; // 过期时间（秒）
    SessionManager*                              sessionManager_;
};

} // namespace session
} // namespace http