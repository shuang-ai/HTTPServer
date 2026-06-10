#include "../../../include/utils/db/DbConnection.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http 
{
namespace db 
{

/**
 * @brief 构造数据库连接对象并建立与MySQL服务器的连接。
 *
 * 该构造函数使用提供的连接参数初始化成员变量，并尝试建立实际的数据库连接。
 * 连接成功后，会配置重连策略、超时时间、禁用多语句执行，并将字符集设置为 utf8mb4。
 * 如果连接失败或配置过程中发生异常，将抛出 DbException。
 *
 * @param host     数据库服务器的主机地址（例如 "localhost" 或 IP 地址）。
 * @param user     用于认证的用户名。
 * @param password 用于认证的密码。
 * @param database 要连接的默认数据库名称。
 *
 * @throws DbException 当数据库连接失败、设置 schema 失败或执行初始化 SQL 语句失败时抛出。
 */
DbConnection::DbConnection(const std::string& host,
                         const std::string& user,
                         const std::string& password,
                         const std::string& database)
    : host_(host)
    , user_(user)
    , password_(password)
    , database_(database)
{
    try 
    {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        conn_.reset(driver->connect(host_, user_, password_));
        if (conn_) 
        {
            conn_->setSchema(database_);
            
            // 设置连接属性
            conn_->setClientOption("OPT_RECONNECT", "true");
            conn_->setClientOption("OPT_CONNECT_TIMEOUT", "10");
            conn_->setClientOption("multi_statements", "false");
            
            // 设置字符集
            std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
            stmt->execute("SET NAMES utf8mb4");
            
            LOG_INFO << "Database connection established";
        }
    } 
    catch (const sql::SQLException& e) 
    {
        LOG_ERROR << "Failed to create database connection: " << e.what();
        throw DbException(e.what());
    }
}

DbConnection::~DbConnection() 
{
    try 
    {
        cleanup();
    } 
    catch (...) 
    {
        // 析构函数中不抛出异常
    }
    LOG_INFO << "Database connection closed";
}

/**
 * @brief 检查数据库连接是否有效。
 * 
 * 通过执行一个简单的查询（SELECT 1）来验证当前数据库连接的可用性。
 * 该操作会创建一个新的语句对象，避免复用可能处于无效状态的缓存语句。
 * 
 * @return true 如果连接有效且查询成功执行。
 * @return false 如果发生 SQL 异常，表明连接已断开或不可用。
 */
bool DbConnection::ping() 
{
    try 
    {
        // 不使用 getStmt，直接创建新的语句
        std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
        return true;
    } 
    catch (const sql::SQLException& e) 
    {
        LOG_ERROR << "Ping failed: " << e.what();
        return false;
    }
}

/**
 * @brief 检查数据库连接是否有效且可用。
 *
 * 该函数通过执行一个简单的 SQL 查询（SELECT 1）来验证当前的数据库连接状态。
 * 如果连接指针为空，或者在执行查询过程中抛出 SQL 异常，则认为连接无效。
 *
 * @return true 如果数据库连接存在且能够成功执行查询。
 * @return false 如果数据库连接为空，或在执行查询时发生异常。
 */
bool DbConnection::isValid() 
{
    try 
    {
        if (!conn_) return false;
        std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
        stmt->execute("SELECT 1");
        return true;
    } 
    catch (const sql::SQLException&) 
    {
        return false;
    }
}

/**
 * @brief 重新连接数据库。
 *
 * 该函数尝试重新建立数据库连接。如果当前连接已存在，则先断开连接。
 * 然后使用相同的主机、用户名和密码重新创建一个新的连接。
 *
 * @note 该函数可能会抛出 DbException 异常，如果重新连接失败。
 */
void DbConnection::reconnect() 
{
    try 
    {
        if (conn_) 
        {
            conn_->reconnect();
        } 
        else 
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            conn_.reset(driver->connect(host_, user_, password_));
            conn_->setSchema(database_);
        }
    } 
    catch (const sql::SQLException& e) 
    {
        LOG_ERROR << "Reconnect failed: " << e.what();
        throw DbException(e.what());
    }
}

/**
 * @brief 清理数据库连接资源，确保连接处于干净且可用的状态。
 * 
 * 该函数执行以下操作：
 * 1. 回滚任何未提交的事务并恢复自动提交模式。
 * 2. 消耗并清理所有未处理的结果集，防止资源泄漏。
 * 3. 如果清理过程中发生异常，尝试重新连接数据库以恢复连接可用性。
 * 
 * @note 此函数是线程安全的，通过互斥锁保护内部状态。
 */
void DbConnection::cleanup() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    try 
    {
        if (conn_) 
        {
            // 确保所有事务都已完成
            if (!conn_->getAutoCommit()) 
            {
                conn_->rollback();
                conn_->setAutoCommit(true);
            }
            
            // 清理所有未处理的结果集
            std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
            while (stmt->getMoreResults()) 
            {
                auto result = stmt->getResultSet();
                while (result && result->next()) 
                {
                    // 消费所有结果
                }
            }
        }
    } 
    catch (const std::exception& e) 
    {
        LOG_WARN << "Error cleaning up connection: " << e.what();
        try 
        {
            reconnect();
        } 
        catch (...) 
        {
            // 忽略重连错误
        }
    }
}

} // namespace db
} // namespace http
