#include "../../include/middleware/MiddlewareChain.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace middleware
{

void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware)
{
    middlewares_.push_back(middleware);
}

/**
 * @brief 执行所有中间件的前置处理逻辑
 * 
 * 遍历注册的中间件列表，依次调用每个中间件的 before 方法，
 * 以便在请求正式处理前进行预处理操作（如权限验证、日志记录等）。
 * 
 * @param request HTTP请求对象的引用，将在各个中间件中被修改或检查
 */
void MiddlewareChain::processBefore(HttpRequest &request)
{
    // 遍历中间件列表并执行前置处理
    for (auto &middleware : middlewares_)
    {
        middleware->before(request);
    }
}

/**
 * @brief 按顺序执行所有中间件的响应后处理逻辑。
 *
 * 遍历中间件链，依次调用每个中间件的 after 方法，
 * 以便在响应被返回给客户端之后进行后续处理（如数据转换、错误处理等）。
 *
 * @param response HTTP 响应对象的引用，中间件可对其进行修改或检查。
 */
void MiddlewareChain::processAfter(HttpResponse &response)
{
    try
    {
        // 反向处理响应，以保持中间件的正确执行顺序
        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {
            if (*it)
            { // 添加空指针检查
                (*it)->after(response);
            }
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error in middleware after processing: " << e.what();
    }
}

} // namespace middleware
} // namespace http
