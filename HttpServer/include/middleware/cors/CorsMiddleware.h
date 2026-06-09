#pragma once

#include "../Middleware.h"
#include "../../http/HttpRequest.h"
#include "../../http/HttpResponse.h"
#include "CorsConfig.h"

namespace http 
{
namespace middleware 
{

class CorsMiddleware : public Middleware 
{
public:
    explicit CorsMiddleware(const CorsConfig& config = CorsConfig::defaultConfig());
    
    // 请求前处理
    void before(HttpRequest& request) override;
    // 响应后处理
    void after(HttpResponse& response) override;

    // 工具函数：将字符串数组连接成单个字符串
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

private:
    // 判断请求源是否被允许
    bool isOriginAllowed(const std::string& origin) const;
    // 处理CORS预检请求
    void handlePreflightRequest(const HttpRequest& request, HttpResponse& response);
    // 为HTTP响应添加CORS头
    void addCorsHeaders(HttpResponse& response, const std::string& origin);

private:
    CorsConfig config_;
};

} // namespace middleware
} // namespace http