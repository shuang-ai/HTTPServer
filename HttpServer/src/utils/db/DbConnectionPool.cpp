#include "../../../include/utils/db/DbConnectionPool.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http 
{
namespace db 
{

void DbConnectionPool::init(const std::string& host,
                          const std::string& user,
                          const std::string& password,
                          const std::string& database,
                          size_t poolSize) 
{
    // 连接池会被多个线程访问，所以操作其成员变量时需要加锁
    std::lock_guard<std::mutex> lock(mutex_);
    // 确保只初始化一次
    if (initialized_) 
    {
        return;
    }

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    // 创建连接
    for (size_t i = 0; i < poolSize; ++i) 
    {
        connections_.push(createConnection());
    }

    initialized_ = true;
    LOG_INFO << "Database connection pool initialized with " << poolSize << " connections";
}

DbConnectionPool::DbConnectionPool() 
{
    checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
    checkThread_.detach();
}

/**
 * @brief 数据库连接池析构函数
 * 
 * 负责清理连接池中剩余的所有数据库连接，并释放相关资源。
 * 在对象生命周期结束时自动调用，确保无资源泄漏。
 */
DbConnectionPool::~DbConnectionPool() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) 
    {
        connections_.pop();
    }
    LOG_INFO << "Database connection pool destroyed";
}

/**
 * @brief 从数据库连接池中获取一个可用的数据库连接。
 * 
 * 该函数会尝试从内部队列中取出一个空闲连接。如果当前没有可用连接，
 * 调用线程将被阻塞直到有连接被释放回池中或发生异常。
 * 获取到的连接在 shared_ptr 销毁时会自动归还到连接池中。
 * 
 * @return std::shared_ptr<DbConnection> 指向可用数据库连接的智能指针。
 *         当该智能指针引用计数归零时，连接将自动返回池中。
 * 
 * @throws DbException 如果连接池尚未初始化。
 * @throws std::exception 如果在获取或验证连接过程中发生错误。
 */
std::shared_ptr<DbConnection> DbConnectionPool::getConnection() 
{
    std::shared_ptr<DbConnection> conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        
        while (connections_.empty()) 
        {
            if (!initialized_) 
            {
                throw DbException("Connection pool not initialized");
            }
            LOG_INFO << "Waiting for available connection...";
            cv_.wait(lock);
        }
        
        conn = connections_.front();
        connections_.pop();
    } // 释放锁
    
    try 
    {
        // 在锁外检查连接
        if (!conn->ping()) 
        {
            LOG_WARN << "Connection lost, attempting to reconnect...";
            conn->reconnect();
        }
        
        // 第二个参数是自定义的删除器，当 shared_ptr 被销毁时会调用该函数将连接返回池中
        return std::shared_ptr<DbConnection>(conn.get(), 
            [this, conn](DbConnection*) {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.push(conn);
                cv_.notify_one();
            });
    } 
    catch (const std::exception& e) 
    {
        LOG_ERROR << "Failed to get connection: " << e.what();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::createConnection() 
{
    return std::make_shared<DbConnection>(host_, user_, password_, database_);
}

/**
 * @brief 定期检查并维护数据库连接池中的连接状态。
 * 
 * 该函数在一个无限循环中运行，主要执行以下任务：
 * 1. 获取当前连接池中的所有连接快照。
 * 2. 在锁外对每个连接进行心跳检测（ping）。
 * 3. 如果连接失效，尝试重新连接。
 * 4. 每次检查间隔60秒，若发生异常则短暂休眠后重试。
 * 
 * @note 此函数通常作为后台守护线程运行，无参数输入，无返回值。
 */
void DbConnectionPool::checkConnections() 
{
    while (true) 
    {
        try 
        {
            std::vector<std::shared_ptr<DbConnection>> connsToCheck;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (connections_.empty()) 
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
                
                auto temp = connections_;
                while (!temp.empty()) 
                {
                    connsToCheck.push_back(temp.front());
                    temp.pop();
                }
            }
            
            // 在锁外检查连接
            for (auto& conn : connsToCheck) 
            {
                if (!conn->ping()) 
                {
                    try 
                    {
                        conn->reconnect();
                    } 
                    catch (const std::exception& e) 
                    {
                        LOG_ERROR << "Failed to reconnect: " << e.what();
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(60));
        } 
        catch (const std::exception& e) 
        {
            LOG_ERROR << "Error in check thread: " << e.what();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

} // namespace db
} // namespace http