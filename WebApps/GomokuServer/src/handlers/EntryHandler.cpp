#include "../include/handlers/EntryHandler.h"

void EntryHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    // 因为是get请求，请求的url也拿到了，我们就可以直接返回响应了
    std::string reqFile;
    reqFile.append("../WebApps/GomokuServer/resource/entry.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid())
    {
        LOG_WARN << reqFile << " not exist";
        fileOperater.resetDefaultFile(); // 404 NOT FOUND
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer); // 读出文件数据
    std::string bufStr = std::string(buffer.data(), buffer.size());
    
    // 设置HTTP响应的状态行信息。
    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
     // 设置连接保持活跃，不关闭连接（Keep-Alive）
    resp->setCloseConnection(false);
     // 设置响应内容类型为 HTML 文本
    resp->setContentType("text/html");
    // 设置响应体内容的长度
    resp->setContentLength(bufStr.size());
    // 设置响应体内容
    resp->setBody(bufStr);
}
