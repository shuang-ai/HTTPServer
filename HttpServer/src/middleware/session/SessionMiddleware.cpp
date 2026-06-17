#include "../../../include/middleware/session/SessionMiddleware.h"

#include <muduo/base/Logging.h>

namespace http
{
namespace middleware
{

SessionMiddleware::SessionMiddleware(session::SessionManager* sessionManager,
                                     const SessionConfig& config)
    : sessionManager_(sessionManager)
    , config_(config)
{}

void SessionMiddleware::before(HttpRequest& request, HttpResponse* response)
{
    if (!sessionManager_)
    {
        LOG_WARN << "SessionMiddleware: SessionManager is null, skip session loading";
        return;
    }

    auto session = sessionManager_->getSession(request, response);
    request.setSession(session);

    if (!config_.enableAuthGuard || config_.isPublicPath(request.path()))
    {
        return;
    }

    if (session->getValue("isLoggedIn") != "true")
    {
        LOG_INFO << "SessionMiddleware: unauthorized access to " << request.path();
        rejectUnauthorized(request);
    }
}

void SessionMiddleware::after(HttpResponse& /*response*/)
{
    // Session 的 Cookie 写入与持久化在 getSession / setValue 中完成
}

void SessionMiddleware::rejectUnauthorized(const HttpRequest& request) const
{
    HttpResponse response;
    response.setStatusLine(request.getVersion(),
                           HttpResponse::k401Unauthorized,
                           "Unauthorized");
    response.setCloseConnection(true);
    response.setContentType("application/json");
    const std::string body = R"({"status":"error","message":"Unauthorized"})";
    response.setContentLength(body.size());
    response.setBody(body);
    throw response;
}

} // namespace middleware
} // namespace http
