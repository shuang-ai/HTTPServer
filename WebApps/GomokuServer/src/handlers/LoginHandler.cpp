#include "../include/handlers/LoginHandler.h"

/**
 * @brief 处理用户登录请求。
 * 
 * 该函数负责解析HTTP请求中的JSON数据，验证用户名和密码，
 * 管理用户会话状态以及在线用户统计。
 * 
 * @param req HTTP请求对象，包含请求头、Body等信息。
 * @param resp HTTP响应对象指针，用于设置响应状态、 headers 和 Body。
 */
void LoginHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // 处理登录逻辑
    // 验证 contentType
    auto contentType = req.getHeader("Content-Type");
    if (contentType.empty() || contentType != "application/json" || req.getBody().empty())
    {
        LOG_INFO << "content" << req.getBody();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }

    // JSON 解析使用 try catch 捕获异常
    try
    {
        json parsed = json::parse(req.getBody());
        std::string username = parsed["username"];
        std::string password = parsed["password"];
        // 验证用户是否存在
        int userId = queryUserId(username, password);
        if (userId != -1)
        {
            auto session = req.getSession();
            session->setValue("userId", std::to_string(userId));
            session->setValue("username", username);
            session->setValue("isLoggedIn", "true");
            // 查看这个用户是否在线
            if (server_->onlineUsers_.find(userId) == server_->onlineUsers_.end() || server_->onlineUsers_[userId] == false)
            {
                {
                    std::lock_guard<std::mutex> lock(server_->mutexForOnlineUsers_);
                    server_->onlineUsers_[userId] = true;
                }
                
                // 更新历史最高在线人数
                server_->updateMaxOnline(server_->onlineUsers_.size());
                // 用户存在登录成功
                // 封装json 数据。
                json successResp;
                successResp["success"] = true;
                successResp["userId"] = userId;
                std::string successBody = successResp.dump(4);

                // 填充http响应
                resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
                resp->setCloseConnection(false);
                resp->setContentType("application/json");
                resp->setContentLength(successBody.size());
                resp->setBody(successBody);
                return;
            }
            else
            {
                // FIXME: 当前该用户正在其他地方登录中，将原有登录用户强制下线更好
                // 不允许重复登录，
                json failureResp;
                failureResp["success"] = false;
                failureResp["error"] = "账号已在其他地方登录";
                std::string failureBody = failureResp.dump(4);

                resp->setStatusLine(req.getVersion(), http::HttpResponse::k403Forbidden, "Forbidden");
                resp->setCloseConnection(true);
                resp->setContentType("application/json");
                resp->setContentLength(failureBody.size());
                resp->setBody(failureBody);
                return;
            }
        }
        else // 账号密码错误，请重新登录
        {
            // 封装json数据
            json failureResp;
            failureResp["status"] = "error";
            failureResp["message"] = "Invalid username or password";
            std::string failureBody = failureResp.dump(4);

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(failureBody.size());
            resp->setBody(failureBody);
            return;
        }
    }
    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);

        // 填充http响应
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
        return;
    }
}

int LoginHandler::queryUserId(const std::string &username, const std::string &password)
{
    // 前端用户传来账号密码，查找数据库是否有该账号密码
    // 使用预处理语句, 防止sql注入
    std::string sql = "SELECT id FROM users WHERE username = ? AND password = ?";
    // std::vector<std::string> params = {username, password};
    sql::ResultSet* res = mysqlUtil_.executeQuery(sql, username, password);
    if (res->next())
    {
        int id = res->getInt("id");
        return id;
    }
    // 如果查询结果为空，则返回-1
    return -1;
}

