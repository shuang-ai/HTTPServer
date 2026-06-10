#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include "DbConnection.h"

namespace http 
{
namespace db 
{

class DbConnectionPool 
{
public:
    // 单例模式
    static DbConnectionPool& getInstance() 
    {
        static DbConnectionPool instance;
        return instance;
    }

    // 初始化连接池
    void init(const std::string& host,
             const std::string& user,
             const std::string& password,
             const std::string& database,
             size_t poolSize = 10);

    /**
     * @brief 获取数据库连接对象
     * 
     * @return std::shared_ptr<DbConnection> 返回一个指向 DbConnection 对象的智能指针
     */
    std::shared_ptr<DbConnection> getConnection();

private:
    // 构造函数
    DbConnectionPool();
    // 析构函数
    ~DbConnectionPool();

    // 禁止拷贝
    DbConnectionPool(const DbConnectionPool&) = delete;
    DbConnectionPool& operator=(const DbConnectionPool&) = delete;

    std::shared_ptr<DbConnection> createConnection();

/**
 * @brief 检查当前的连接状态。
 *
 * 该函数用于验证系统或网络中的连接是否正常，确保通信链路的可用性。
 *
 * @param 无
 * @return 无
 */
    void checkConnections(); 

private:
    std::string                               host_;
    std::string                               user_;
    std::string                               password_;
    std::string                               database_;
    std::queue<std::shared_ptr<DbConnection>> connections_;
    std::mutex                                mutex_;
    std::condition_variable                   cv_;
    bool                                      initialized_ = false;
    std::thread                               checkThread_; // 添加检查线程
};

} // namespace db
} // namespace http
