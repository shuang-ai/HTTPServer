#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include "DbException.h"

namespace http 
{
namespace db 
{

class DbConnection 
{
public:
    /**
     * @brief 构造数据库连接对象
     * 
     * @param host 数据库主机地址
     * @param user 数据库用户名
     * @param password 数据库密码
     * @param database 数据库名称
     */
    DbConnection(const std::string& host, 
                const std::string& user,
                const std::string& password,
                const std::string& database);
    ~DbConnection();

    // 禁止拷贝
    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

    /**
     * @brief 检查当前连接或对象状态是否有效。
     * 
     * @return true 如果状态有效。
     * @return false 如果状态无效。
     */
    bool isValid();

    /**
     * @brief 尝试重新建立连接。
     * 
     * 当检测到连接断开或失效时，调用此函数以恢复连接状态。
     */
    void reconnect();

    /**
     * @brief 清理资源并释放占用的内存或句柄。
     * 
     * 在对象销毁或需要重置状态时调用，确保没有资源泄漏。
     */
    void cleanup();

    /**
     * @brief 执行SQL查询(select)并返回结果集
     * 
     * 该函数使用互斥锁保证线程安全，创建预处理语句并绑定参数后执行查询。
     * 如果查询失败，将记录错误日志并抛出DbException异常。
     * 
     * @tparam Args 可变参数模板类型，用于传递SQL查询所需的参数
     * @param sql SQL查询字符串
     * @param args 传递给SQL查询的参数包，将被绑定到预处理语句中
     * @return sql::ResultSet* 指向查询结果集的指针，由调用者负责管理生命周期
     * @throws DbException 当SQL执行失败时抛出此异常
     */
    template<typename... Args>
    sql::ResultSet* executeQuery(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try 
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeQuery();
        } 
        catch (const sql::SQLException& e) 
        {
            LOG_ERROR << "Query failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }
    
    /**
     * @brief 执行SQL更新操作（INSERT, UPDATE, DELETE等）。
     * 
     * 该函数通过可变参数模板支持动态绑定SQL参数，并在执行前获取互斥锁以保证线程安全。
     * 每次调用都会创建新的预处理语句，不使用缓存机制。
     * 
     * @tparam Args 可变参数类型包，用于匹配SQL语句中的占位符参数。
     * @param sql SQL更新语句字符串，可能包含占位符（如 ?）。
     * @param args 要绑定到SQL语句占位符的参数列表，支持完美转发。
     * @return int 受SQL语句影响的行数。
     * @throw DbException 当数据库操作发生异常时抛出，包含原始错误信息。
     */
    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try 
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeUpdate();
        } 
        catch (const sql::SQLException& e) 
        {
            LOG_ERROR << "Update failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }

    /**
    * @brief 检测连接是否有效
    * 
    * @return true 连接有效
    * @return false 连接无效
    */
    bool ping();  
private:
     // 辅助函数：递归终止条件
    void bindParams(sql::PreparedStatement*, int) {}
    
    // 辅助函数：绑定参数
    template<typename T, typename... Args>
    void bindParams(sql::PreparedStatement* stmt, int index, 
                   T&& value, Args&&... args) 
    {
        stmt->setString(index, std::to_string(std::forward<T>(value)));
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }
    
    // 特化 string 类型的参数绑定
    template<typename... Args>
    void bindParams(sql::PreparedStatement* stmt, int index, 
                   const std::string& value, Args&&... args) 
    {
        stmt->setString(index, value);
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<sql::Connection> conn_;
    std::string                      host_;
    std::string                      user_;
    std::string                      password_;
    std::string                      database_;
    std::mutex                       mutex_;
};

} // namespace db
} // namespace http