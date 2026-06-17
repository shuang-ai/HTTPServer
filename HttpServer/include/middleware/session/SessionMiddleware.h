#pragma once

#include "../Middleware.h"
#include "../../session/SessionManager.h"
#include "SessionConfig.h"

namespace http
{
namespace middleware
{

/**
 * @brief Session 中间件
 *
 * 在请求进入路由处理器之前，自动从 Cookie 加载或创建 Session，
 * 并挂载到 HttpRequest 上；可选地对非公开路径进行登录鉴权。
 */
class SessionMiddleware : public Middleware
{
public:
    SessionMiddleware(session::SessionManager* sessionManager,
                      const SessionConfig& config = SessionConfig::defaultConfig());

    void before(HttpRequest& request, HttpResponse* response) override;
    void after(HttpResponse& response) override;

private:
    void rejectUnauthorized(const HttpRequest& request) const;

    session::SessionManager* sessionManager_;
    SessionConfig            config_;
};

} // namespace middleware
} // namespace http
