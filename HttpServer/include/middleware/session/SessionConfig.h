#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace http
{
namespace middleware
{

/**
 * @brief Session 中间件配置
 */
struct SessionConfig
{
    bool enableAuthGuard = false;
    std::vector<std::string> publicPaths;

    bool isPublicPath(const std::string& path) const
    {
        return std::find(publicPaths.begin(), publicPaths.end(), path) != publicPaths.end();
    }

    static SessionConfig defaultConfig()
    {
        return SessionConfig{};
    }
};

} // namespace middleware
} // namespace http
