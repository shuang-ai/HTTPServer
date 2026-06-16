#pragma once

#include <string>
#include <vector>

namespace http 
{
namespace middleware 
{

/**
 * @brief 跨域资源共享（CORS）配置结构体
 * 
 * 该结构体用于定义 HTTP 中间件中 CORS 策略的各项参数，
 * 包括允许的源、方法、头部以及凭证支持和预检请求缓存时间。
 */
struct CorsConfig 
{
    std::vector<std::string> allowedOrigins;
    std::vector<std::string> allowedMethods;
    std::vector<std::string> allowedHeaders;
    bool allowCredentials = false;
    int maxAge = 3600;
    
    /**
     * @brief 生成默认的 CORS 配置
     * 
     * 返回一个包含常用默认值的 CorsConfig 实例：
     * - 允许所有源 ("*")
     * - 允许常见 HTTP 方法 (GET, POST, PUT, DELETE, OPTIONS)
     * - 允许常见头部 (Content-Type, Authorization)
     * - 不允许携带凭证
     * - 预检缓存时间为 3600 秒
     * 
     * @return CorsConfig 默认配置对象
     */
    static CorsConfig defaultConfig() 
    {
        CorsConfig config;
        config.allowedOrigins = {"*"};
        config.allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
        config.allowedHeaders = {"Content-Type", "Authorization"};
        return config;
    }
};

} // namespace middleware
} // namespace http