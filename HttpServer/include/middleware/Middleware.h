#pragma once

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http 
{
namespace middleware 
{

/**
 * @brief HTTP 中间件基类
 * 
 * 定义了中间件的通用接口，支持在请求处理前和响应处理后执行自定义逻辑。
 * 支持链式调用，通过设置下一个中间件形成责任链。
 */
class Middleware 
{
public:
    virtual ~Middleware() = default;
    
    // 请求前处理
    virtual void before(HttpRequest& request) = 0;
    
    // 响应后处理
    virtual void after(HttpResponse& response) = 0;
    
    // 设置下一个中间件
    void setNext(std::shared_ptr<Middleware> next) 
    {
        nextMiddleware_ = next;
    }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};

} // namespace middleware
} // namespace http