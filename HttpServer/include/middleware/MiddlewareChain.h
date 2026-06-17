#pragma once

#include <vector>
#include <memory>
#include "Middleware.h"

namespace http 
{
namespace middleware 
{

class MiddlewareChain 
{
public:
/**
 * @brief 向中间件链中添加一个中间件实例。
 *
 * @param middleware 要添加的中间件智能指针。
 */
    void addMiddleware(std::shared_ptr<Middleware> middleware);

/**
 * @brief 按顺序执行所有中间件的请求前处理逻辑。
 *
 * 遍历中间件链，依次调用每个中间件的 before 方法，
 * 以便在请求被正式处理之前进行预处理（如鉴权、日志记录等）。
 *
 * @param request HTTP 请求对象的引用，中间件可对其进行修改或检查。
 */
    void processBefore(HttpRequest& request, HttpResponse* response);

/**
 * @brief 按顺序执行所有中间件的响应后处理逻辑。
 *
 * 遍历中间件链，依次调用每个中间件的 after 方法，
 * 以便在响应被返回给客户端之后进行后续处理（如数据转换、错误处理等）。
 *
 * @param response HTTP 响应对象的引用，中间件可对其进行修改或检查。
 */
    void processAfter(HttpResponse& response);

private:
    std::vector<std::shared_ptr<Middleware>> middlewares_;
};

} // namespace middleware
} // namespace http