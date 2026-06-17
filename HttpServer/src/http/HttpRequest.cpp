#include "../../include/http/HttpRequest.h"
#include "../../include/session/Session.h"

namespace http
{

/**
 * @brief 设置HTTP请求的接收时间戳
 * 
 * @param t 接收时间戳，表示该HTTP请求被服务器接收到的具体时刻
 */
void HttpRequest::setReceiveTime(muduo::Timestamp t)
{
    receiveTime_ = t;
}

/**
 * @brief 根据给定的字符串范围设置 HTTP 请求方法。
 * 
 * 该函数解析[start, end)范围内的字符序列，将其与标准的 HTTP 方法名称进行匹配，
 * 并更新内部成员变量 method_。如果匹配成功且方法有效，则返回 true；否则将 method_
 * 设置为 kInvalid 并返回 false。
 * 
 * @param start 指向 HTTP 方法名称起始位置的指针。
 * @param end   指向 HTTP 方法名称结束位置的指针（不包含该位置字符）。
 * @return true  如果成功识别为有效的 HTTP 方法（GET, POST, PUT, DELETE, OPTIONS）。
 * @return false 如果无法识别或方法无效。
 */
bool HttpRequest::setMethod(const char *start, const char *end)
{
    assert(method_ == kInvalid);
    std::string m(start, end); // [start, end)
    if (m == "GET")
    {
        method_ = kGet;
    }
    else if (m == "POST")
    {
        method_ = kPost;
    }
    else if (m == "PUT")
    {
        method_ = kPut;
    }
    else if (m == "DELETE")
    {
        method_ = kDelete;
    }
    else if (m == "OPTIONS")
    {
        method_ = kOptions;
    }
    else
    {
        method_ = kInvalid;
    }

    return method_ != kInvalid;
}

void HttpRequest::setPath(const char *start, const char *end)
{
    path_.assign(start, end);
}

/**
 * @brief 设置HTTP请求的路径参数。
 * 
 * 将指定的键值对存储到路径参数映射中。如果键已存在，则覆盖其对应的值。
 * 
 * @param key 路径参数的键名。
 * @param value 路径参数的值。
 */
void HttpRequest::setPathParameters(const std::string &key, const std::string &value)
{
    pathParameters_[key] = value;
}

/**
 * @brief 获取指定键名的路径参数值
 * 
 * @param key 路径参数的键名
 * @return std::string 如果找到对应的路径参数则返回其值，否则返回空字符串
 */
std::string HttpRequest::getPathParameters(const std::string &key) const
{
    auto it = pathParameters_.find(key);
    if (it != pathParameters_.end())
    {
        return it->second;
    }
    return "";
}

/**
 * @brief 获取指定键名的查询参数值
 * 
 * @param key 查询参数的键名
 * @return std::string 对应的参数值，如果键不存在则返回空字符串
 */
std::string HttpRequest::getQueryParameters(const std::string &key) const
{
    auto it = queryParameters_.find(key);
    if (it != queryParameters_.end())
    {
        return it->second;
    }
    return "";
}

/**
 * @brief 解析并设置 HTTP 请求的查询参数。
 *
 * 该函数从指定的字符范围中提取查询字符串，按照 '&' 分隔符拆分多个键值对，
 * 再按照 '=' 分隔符提取键和值，最终存入 queryParameters_ 映射中。
 *
 * @param start 指向查询字符串起始位置的指针。
 * @param end   指向查询字符串结束位置的指针（不包含该位置字符）。
 */
void HttpRequest::setQueryParameters(const char *start, const char *end)
{
    std::string argumentStr(start, end);
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;

    // 按 & 分割多个参数
    while ((pos = argumentStr.find('&', prev)) != std::string::npos)
    {
        std::string pair = argumentStr.substr(prev, pos - prev);
        std::string::size_type equalPos = pair.find('=');

        if (equalPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            queryParameters_[key] = value;
        }

        prev = pos + 1;
    }

    // 处理最后一个参数
    std::string lastPair = argumentStr.substr(prev);
    std::string::size_type equalPos = lastPair.find('=');
    if (equalPos != std::string::npos)
    {
        std::string key = lastPair.substr(0, equalPos);
        std::string value = lastPair.substr(equalPos + 1);
        queryParameters_[key] = value;
    }
}

/**
 * @brief 解析并添加一个 HTTP 请求头字段到 headers_ 映射中。
 *
 * 该函数从原始报文片段中提取键值对，处理冒号分隔符，
 * 跳过键后的前导空格，并去除值部分的尾部空格。
 *
 * @param start 指向当前 header 行起始位置的指针（即 key 的开始）。
 * @param colon 指向 key 和 value 之间冒号 ':' 位置的指针。
 * @param end   指向当前 header 行结束位置的指针（通常为 \r\n 之前）。
 */
void HttpRequest::addHeader(const char *start, const char *colon, const char *end)
{
    std::string key(start, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1])) // 消除尾部空格
    {
        value.resize(value.size() - 1);
    }
    headers_[key] = value;
}

/**
 * @brief 获取HTTP请求中指定字段的头部值
 * 
 * @param field 要查询的HTTP头部字段名称
 * @return std::string 对应字段的头部值，如果字段不存在则返回空字符串
 */
std::string HttpRequest::getHeader(const std::string &field) const
{
    std::string result;
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        result = it->second;
    }
    return result;
}

/**
 * @brief 交换当前 HttpRequest 对象与另一个 HttpRequest 对象的内容。
 *
 * 该函数通过逐个交换成员变量，实现两个 HttpRequest 对象之间的高效内容互换。
 * 通常用于移动语义或优化资源管理场景。
 *
 * @param that 要与之交换内容的另一个 HttpRequest 对象的引用。
 * @return 无返回值。
 */
void HttpRequest::swap(HttpRequest &that)
{
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(version_, that.version_);
    std::swap(headers_, that.headers_);
    std::swap(receiveTime_, that.receiveTime_);
    std::swap(session_, that.session_);
}

} // namespace http