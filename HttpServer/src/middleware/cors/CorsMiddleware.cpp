#include "../../../include/middleware/cors/CorsMiddleware.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <muduo/base/Logging.h>

namespace http 
{
namespace middleware 
{

CorsMiddleware::CorsMiddleware(const CorsConfig& config) : config_(config) {}

/**
 * @brief 在请求处理前执行 CORS 中间件逻辑。
 * 
 * 该函数主要负责处理跨域资源共享（CORS）相关的预处理工作。
 * 如果检测到当前请求为 OPTIONS 方法的预检请求，则会构造相应的响应并抛出，
 * 以中断后续的业务逻辑处理，直接返回预检结果。
 * 
 * @param request HTTP 请求对象的引用，用于获取请求方法和相关信息。
 * @return void 无返回值。若为预检请求，通过抛出 HttpResponse 对象来终止流程。
 */
void CorsMiddleware::before(HttpRequest& request) 
{
    LOG_DEBUG << "CorsMiddleware::before - Processing request";
    
    if (request.method() == HttpRequest::Method::kOptions) 
    {
        LOG_INFO << "Processing CORS preflight request";
        HttpResponse response;
        handlePreflightRequest(request, response);
        throw response;
    }
}

/**
 * @brief 在响应发送后处理 CORS 头信息。
 * 
 * 根据配置中的允许源列表，向 HTTP 响应中添加相应的 CORS 头。
 * 如果配置中包含通配符 "*"，则允许所有源；否则使用配置中的第一个允许源。
 * 
 * @param response HTTP 响应对象引用，用于添加 CORS 头信息。
 */
void CorsMiddleware::after(HttpResponse& response) 
{
    LOG_DEBUG << "CorsMiddleware::after - Processing response";
    
    // 直接添加CORS头，简化处理逻辑
    if (!config_.allowedOrigins.empty()) 
    {
        // 如果允许所有源
        if (std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") 
            != config_.allowedOrigins.end()) 
        {
            addCorsHeaders(response, "*");
        } 
        else 
        {
            // 添加第一个允许的源
            addCorsHeaders(response, config_.allowedOrigins[0]);
        }
    }
}

/**
 * @brief 检查给定的 Origin 是否在允许列表中。
 *
 * 判断逻辑如下：
 * 1. 如果允许列表为空，则视为允许所有来源。
 * 2. 如果允许列表中包含通配符 "*"，则视为允许所有来源。
 * 3. 如果允许列表中精确匹配给定的 origin，则允许。
 *
 * @param origin 需要检查的源地址字符串。
 * @return bool 如果源地址被允许，返回 true；否则返回 false。
 */
bool CorsMiddleware::isOriginAllowed(const std::string& origin) const 
{
    return config_.allowedOrigins.empty() || 
           std::find(config_.allowedOrigins.begin(), 
                    config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end() ||
           std::find(config_.allowedOrigins.begin(), 
                    config_.allowedOrigins.end(), origin) != config_.allowedOrigins.end();
}

/**
 * @brief 处理 CORS 预检请求（OPTIONS 请求）。
 * 
 * 该函数负责验证请求来源（Origin）的合法性，并在验证通过后
 * 添加相应的 CORS 响应头，最后返回 204 No Content 状态码。
 * 
 * @param request 只读的 HTTP 请求对象，用于提取 Origin 头等信息。
 * @param response HTTP 响应对象，用于设置状态码和响应头。
 */
void CorsMiddleware::handlePreflightRequest(const HttpRequest& request, 
                                          HttpResponse& response) 
{
    const std::string& origin = request.getHeader("Origin");
    
    if (!isOriginAllowed(origin)) 
    {
        LOG_WARN << "Origin not allowed: " << origin;
        response.setStatusCode(HttpResponse::k403Forbidden);
        return;
    }

    addCorsHeaders(response, origin);
    response.setStatusCode(HttpResponse::k204NoContent);
    LOG_INFO << "Preflight request processed successfully";
}

/**
 * @brief 向 HTTP 响应中添加跨域资源共享（CORS）相关的头部信息。
 * 
 * 该函数根据当前的配置对象（config_）和指定的源（origin），
 * 设置允许的来源、凭证、方法、头部以及预检请求的缓存时间。
 * 如果在添加头部的过程中发生异常，将记录错误日志。
 *
 * @param response 引用类型的 HttpResponse 对象，用于添加 CORS 头部。
 * @param origin 字符串类型，表示允许访问的源地址，将设置为 Access-Control-Allow-Origin 的值。
 * @return 无返回值。
 */
void CorsMiddleware::addCorsHeaders(HttpResponse& response, 
                                  const std::string& origin) 
{
    try 
    {
        response.addHeader("Access-Control-Allow-Origin", origin);
        
        if (config_.allowCredentials) 
        {
            response.addHeader("Access-Control-Allow-Credentials", "true");
        }
        
        if (!config_.allowedMethods.empty()) 
        {
            response.addHeader("Access-Control-Allow-Methods", 
                             join(config_.allowedMethods, ", "));
        }
        
        if (!config_.allowedHeaders.empty()) 
        {
            response.addHeader("Access-Control-Allow-Headers", 
                             join(config_.allowedHeaders, ", "));
        }
        
        response.addHeader("Access-Control-Max-Age", 
                          std::to_string(config_.maxAge));
        
        LOG_DEBUG << "CORS headers added successfully";
    } 
    catch (const std::exception& e) 
    {
        LOG_ERROR << "Error adding CORS headers: " << e.what();
    }
}

// 工具函数：将字符串数组连接成单个字符串
std::string CorsMiddleware::join(const std::vector<std::string>& strings, const std::string& delimiter) 
{
    std::ostringstream result;
    for (size_t i = 0; i < strings.size(); ++i) 
    {
        if (i > 0) result << delimiter;
        result << strings[i];
    }
    return result.str();
}

} // namespace middleware
} // namespace http