 #pragma once
 #include "db/DbConnectionPool.h"
 
#include <string>

namespace http
{

/**
 * @brief MySQL 数据库工具类，提供基于连接池的数据库操作接口。
 *
 * 该类封装了数据库连接池的初始化以及常用的查询和更新操作，
 * 支持可变参数模板以方便构建动态 SQL 语句。
 */
class MysqlUtil
{
public:
    /**
     * @brief 初始化数据库连接池。
     *
     * @param host      数据库主机地址。
     * @param user      数据库用户名。
     * @param password  数据库密码。
     * @param database  要连接的数据库名称。
     * @param poolSize  连接池大小，默认为 10。
     */
    static void init(const std::string& host, const std::string& user,
                    const std::string& password, const std::string& database,
                    size_t poolSize = 10)
    {
        http::db::DbConnectionPool::getInstance().init(
            host, user, password, database, poolSize);
    }

    /**
     * @brief 执行 SQL 查询语句(select)。
     *
     * @tparam Args   可变参数类型，用于填充 SQL 语句中的占位符。
     * @param sql     SQL 查询语句字符串。
     * @param args    传递给 SQL 语句的参数包。
     * @return sql::ResultSet* 指向结果集的指针，由调用者负责管理生命周期或根据底层库约定处理。
     */
    template<typename... Args>
    sql::ResultSet* executeQuery(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeQuery(sql, std::forward<Args>(args)...);
    }

    /**
     * @brief 执行 SQL 更新语句(update/delete/insert)。
     *
     * @tparam Args   可变参数类型，用于填充 SQL 语句中的占位符。
     * @param sql     SQL 更新语句字符串。
     * @param args    传递给 SQL 语句的参数包。
     * @return int    返回受影响的行数。
     */
    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeUpdate(sql, std::forward<Args>(args)...);
    }
};

} // namespace http
