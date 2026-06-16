#pragma once

#include "../Middleware.h"
#include "../../http/HttpRequest.h"
#include "../../http/HttpResponse.h"
#include "CorsConfig.h"

namespace http 
{
namespace middleware 
{

/**
 * @brief 跨域资源共享（CORS）中间件。
 * 
 * 该中间件负责处理 HTTP 请求中的 CORS 相关逻辑，包括验证请求源、
 * 处理预检请求（OPTIONS）以及在响应中添加必要的 CORS 头部信息。
 */
class CorsMiddleware : public Middleware 
{
public:
    /**
     * @brief 构造 CorsMiddleware 实例。
     * 
     * @param config CORS 配置对象，指定允许的源、方法、头部等规则。
     *               如果未提供，则使用默认配置。
     */
    explicit CorsMiddleware(const CorsConfig& config = CorsConfig::defaultConfig());
    
    /**
     * @brief 在HTTP请求处理之前执行的预处理逻辑。
     * 
     * 该函数用于在正式处理请求前对请求对象进行检查、修改或增强，
     * 例如身份验证、日志记录、参数校验等。
     * 
     * @param request 引用类型的HttpRequest对象，表示当前 incoming 的HTTP请求，
     *                可在此阶段对其进行读取或修改。
     */
    void before(HttpRequest& request) override;
    
    /**
     * @brief 在HTTP响应生成之后执行的后处理逻辑。
     * 
     * 该函数用于在响应发送回客户端之前对响应对象进行最终调整，
     * 例如添加通用响应头、记录响应状态、清理资源等。
     * 
     * @param response 引用类型的HttpResponse对象，表示即将发出的HTTP响应，
     *                 可在此阶段对其进行读取或修改。
     */
    void after(HttpResponse& response) override;

    /**
     * @brief 将字符串数组连接成单个字符串
     * 
     * @param strings 待连接的字符串向量
     * @param delimiter 用于分隔各个字符串的分隔符
     * @return std::string 连接后的完整字符串
     */
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

private:
    /**
     * @brief 判断请求源是否被允许
     * 
     * 检查给定的 Origin 是否在允许的跨域来源列表中。
     * 
     * @param origin 请求头中的 Origin 字段值
     * @return true 如果该来源被允许
     * @return false 如果该来源不被允许
     */
    bool isOriginAllowed(const std::string& origin) const;

    /**
     * @brief 处理CORS预检请求
     * 
     * 解析 OPTIONS 预检请求，验证请求方法、头部等信息，
     * 并构建相应的 CORS 响应头返回给客户端。
     * 
     * @param request HTTP 预检请求对象
     * @param response HTTP 响应对象，用于设置状态码和响应头
     */
    void handlePreflightRequest(const HttpRequest& request, HttpResponse& response);

    /**
     * @brief 为HTTP响应添加CORS头
     * 
     * 向 HTTP 响应中注入必要的跨域资源共享（CORS）头部信息，
     * 如 Access-Control-Allow-Origin 等。
     * 
     * @param response HTTP 响应对象
     * @param origin 允许的来源地址
     */
    void addCorsHeaders(HttpResponse& response, const std::string& origin);

private:
    CorsConfig config_;
};

} // namespace middleware
} // namespace http