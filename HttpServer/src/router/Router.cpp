#include "../../include/router/Router.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace router
{

void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    RouteKey key{method, path};
    handlers_[key] = std::move(handler);
}

void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
{
    RouteKey key{method, path};
    callbacks_[key] = std::move(callback);
}

/**
 * @brief 根据HTTP请求的方法和路径路由到相应的处理器或回调函数。
 *
 * 该函数按照以下优先级顺序查找匹配的处理逻辑：
 * 1. 静态路由处理器 (handlers_)
 * 2. 静态路由回调函数 (callbacks_)
 * 3. 动态路由处理器 (regexHandlers_)，支持路径参数提取
 * 4. 动态路由回调函数 (regexCallbacks_)，支持路径参数提取
 *
 * @param req HTTP请求对象，包含方法、路径等信息。
 * @param resp HTTP响应对象指针，用于填充响应数据。
 * @return bool 如果找到匹配的路由并成功处理则返回 true，否则返回 false。
 */
bool Router::route(const HttpRequest &req, HttpResponse *resp)
{
    RouteKey key{req.method(), req.path()};

    // 查找处理器
    auto handlerIt = handlers_.find(key);
    if (handlerIt != handlers_.end())
    {
        handlerIt->second->handle(req, resp);
        return true;
    }

    // 查找回调函数
    auto callbackIt = callbacks_.find(key);
    if (callbackIt != callbacks_.end())
    {
        callbackIt->second(req, resp);
        return true;
    }

    // 查找动态路由处理器
    for (const auto &[method, pathRegex, handler] : regexHandlers_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行处理器
        if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
            // Extract path parameters and add them to the request
            HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
            extractPathParameters(match, newReq);
            
            handler->handle(newReq, resp);
            return true;
        }
    }

    // 查找动态路由回调函数
    for (const auto &[method, pathRegex, callback] : regexCallbacks_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行回调函数
        if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
             // Extract path parameters and add them to the request
            HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
            extractPathParameters(match, newReq);

            callback(req, resp);
            return true;
        }
    }

    return false;
}

} // namespace router
} // namespace http