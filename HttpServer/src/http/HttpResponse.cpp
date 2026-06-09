#include "../../include/http/HttpResponse.h"

namespace http
{

/**
 * @brief 将HTTP响应报文序列化并追加到输出缓冲区中。
 *
 * 该函数按照HTTP协议格式，依次将状态行、响应头字段和消息体写入指定的Buffer。
 * 状态行包含HTTP版本、状态码和状态描述；响应头包括连接控制字段（Connection）
 * 以及用户自定义的头部字段；最后附加消息体内容。
 *
 * @param outputBuf 指向目标输出缓冲区的指针，用于存储序列化后的HTTP响应数据。
 */
void HttpResponse::appendToBuffer(muduo::net::Buffer* outputBuf) const
{
    // 格式化状态行中的HTTP版本和状态码部分
    char buf[32]; 
    // 为什么不把状态信息放入格式化字符串中，因为状态信息有长有短，不方便定义一个固定大小的内存存储
    snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);
    
    outputBuf->append(buf);
    outputBuf->append(statusMessage_);
    outputBuf->append("\r\n");

    // 根据连接策略添加Connection头部字段
    if (closeConnection_) // 思考一下这些地方是不是可以直接移入近headers_中
    {
        outputBuf->append("Connection: close\r\n");
    }
    else
    {
        //snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        //outputBuf->append(buf);
        outputBuf->append("Connection: Keep-Alive\r\n");
    }

    // 遍历并追加所有自定义响应头字段
    for (const auto& header : headers_)
    { // 为什么这里不用格式化字符串？因为key和value的长度不定
        outputBuf->append(header.first);
        outputBuf->append(": "); 
        outputBuf->append(header.second);
        outputBuf->append("\r\n");
    }
    // 添加空行以分隔头部和消息体
    outputBuf->append("\r\n");
    
    // 追加消息体内容
    outputBuf->append(body_);
}

/**
 * @brief 设置HTTP响应的状态行信息。
 *
 * 该函数用于初始化或更新HTTP响应的状态行，包括HTTP协议版本、状态码
 * 以及对应的状态描述消息。
 *
 * @param version HTTP协议版本字符串，例如 "HTTP/1.1"。
 * @param statusCode HTTP状态码枚举值，例如 200, 404, 500 等。
 * @param statusMessage 与状态码对应的文本描述，例如 "OK", "Not Found" 等。
 */
void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

} // namespace http