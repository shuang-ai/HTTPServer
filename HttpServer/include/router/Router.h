#pragma once
#include <iostream>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <regex>
#include <vector>

#include "RouterHandler.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http
{
namespace router
{

/**
 * @brief HTTP 路由器类，负责根据请求方法和 URI 分发请求到对应的处理器。
 *
 * 支持两种注册方式：
 * 1. 对象式处理器 (HandlerPtr)：适合复杂逻辑，可封装多个相关函数和状态。
 * 2. 回调函数式处理器 (HandlerCallback)：适合简单逻辑，直接绑定函数。
 *
 * 支持两种匹配模式：
 * 1. 精准匹配：完全匹配 HTTP 方法和路径。
 * 2. 动态路由（正则匹配）：支持路径参数占位符（如 /:id），内部转换为正则表达式进行匹配。
 */
class Router
{
public:
    using HandlerPtr = std::shared_ptr<RouterHandler>;
    using HandlerCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

    /**
     * @brief 路由键结构体，用于在哈希表中唯一标识一个路由。
     *
     * 由 HTTP 请求方法和 URI 路径组成。
     */
    struct RouteKey
    {
        HttpRequest::Method method;
        std::string path;

        bool operator==(const RouteKey &other) const
        {
            return method == other.method && path == other.path;
        }
    };

    /**
     * @brief RouteKey 的哈希函数对象。
     *
     * 用于 std::unordered_map 计算键的哈希值。
     * 结合 method 和 path 的哈希值生成最终哈希。
     */
    struct RouteKeyHash
    {
        /**
         * @brief 计算 RouteKey 的哈希值。
         *
         * @param key 待计算哈希的路由键。
         * @return size_t 计算得到的哈希值。
         */
        size_t operator()(const RouteKey &key) const
        {
            size_t methodHash = std::hash<int>{}(static_cast<int>(key.method));
            size_t pathHash = std::hash<std::string>{}(key.path);
            return methodHash * 31 + pathHash;
        }
    };

    /**
     * @brief 注册对象式路由处理器（精准匹配）。
     *
     * @param method HTTP 请求方法（如 GET, POST 等）。
     * @param path 请求路径，必须完全匹配。
     * @param handler 指向 RouterHandler 对象的共享指针。
     */
    void registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler);

    /**
     * @brief 注册回调函数式处理器（精准匹配）。
     *
     * @param method HTTP 请求方法（如 GET, POST 等）。
     * @param path 请求路径，必须完全匹配。
     * @param callback 处理请求的回调函数。
     */
    void registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback);

    /**
     * @brief 注册动态路由处理器（正则匹配）。
     *
     * 路径中包含 ":param" 格式的占位符将被转换为正则捕获组。
     *
     * @param method HTTP 请求方法。
     * @param path 包含路径参数的模式字符串（例如 "/users/:id"）。
     * @param handler 指向 RouterHandler 对象的共享指针。
     */
    void addRegexHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
    {
        std::regex pathRegex = convertToRegex(path);
        regexHandlers_.emplace_back(method, pathRegex, handler);
    }

    /**
     * @brief 注册动态路由处理函数（正则匹配）。
     *
     * 路径中包含 ":param" 格式的占位符将被转换为正则捕获组。
     *
     * @param method HTTP 请求方法。
     * @param path 包含路径参数的模式字符串（例如 "/users/:id"）。
     * @param callback 处理请求的回调函数。
     */
    void addRegexCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
    {
        std::regex pathRegex = convertToRegex(path);
        regexCallbacks_.emplace_back(method, pathRegex, callback);
    }

    /**
     * @brief 根据请求信息路由到对应的处理器并执行。
     *
     * 首先尝试精准匹配，若未找到则尝试正则匹配。
     * 匹配成功后，如果是动态路由，会提取路径参数并设置到 request 对象中。
     *
     * @param req HTTP 请求对象引用。
     * @param resp HTTP 响应对象指针，用于写入响应数据。
     * @return bool 如果找到匹配的处理器并成功执行返回 true，否则返回 false。
     */
    bool route(const HttpRequest &req, HttpResponse *resp);

private:
    /**
     * @brief 将路径模式字符串转换为标准正则表达式。
     *
     * 将路径中的 "/:paramName" 替换为 "/([^/]+)" 以匹配任意非斜杠字符序列。
     *
     * @param pathPattern 原始路径模式字符串。
     * @return std::regex 转换后的正则表达式对象。
     */
    std::regex convertToRegex(const std::string &pathPattern)
    { // 将路径模式转换为正则表达式，支持匹配任意路径参数
        std::string regexPattern = "^" + std::regex_replace(pathPattern, std::regex(R"(/:([^/]+))"), R"(/([^/]+))") + "$";
        return std::regex(regexPattern);
    }

    /**
     * @brief 从正则匹配结果中提取路径参数并设置到请求对象中。
     *
     * 参数名默认命名为 "param1", "param2" 等，对应正则捕获组的顺序。
     *
     * @param match 正则匹配结果对象。
     * @param request HTTP 请求对象引用，用于存储提取的参数。
     */
    void extractPathParameters(const std::smatch &match, HttpRequest &request)
    {
        // Assuming the first match is the full path, parameters start from index 1
        for (size_t i = 1; i < match.size(); ++i)
        {
            request.setPathParameters("param" + std::to_string(i), match[i].str());
        }
    }

private:
    /**
     * @brief 封装动态路由回调函数的结构体。
     */
    struct RouteCallbackObj
    {
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerCallback callback_;
        RouteCallbackObj(HttpRequest::Method method, std::regex pathRegex, const HandlerCallback &callback)
            : method_(method), pathRegex_(pathRegex), callback_(callback) {}
    };

    /**
     * @brief 封装动态路由处理器的结构体。
     */
    struct RouteHandlerObj
    {
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerPtr handler_;
        RouteHandlerObj(HttpRequest::Method method, std::regex pathRegex, HandlerPtr handler)
            : method_(method), pathRegex_(pathRegex), handler_(handler) {}
    };

    std::unordered_map<RouteKey, HandlerPtr, RouteKeyHash>      handlers_;       // 精准匹配
    std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> callbacks_; // 精准匹配
    std::vector<RouteHandlerObj>                                regexHandlers_;     // 正则匹配
    std::vector<RouteCallbackObj>                               regexCallbacks_;   // 正则匹配
};


} // namespace router
} // namespace http