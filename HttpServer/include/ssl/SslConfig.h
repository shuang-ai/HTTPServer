#pragma once
#include "SslTypes.h"
#include <string>
#include <vector>

namespace ssl 
{

/**
 * @brief SSL/TLS 配置类，用于管理 SSL 连接所需的各项参数。
 *
 * 该类封装了证书、私钥、协议版本、加密套件、客户端验证以及会话管理等配置项。
 */
class SslConfig 
{
public:
    SslConfig();
    ~SslConfig() = default;

    /**
     * @brief 设置证书文件路径。
     *
     * @param certFile 证书文件的路径字符串。
     */
    void setCertificateFile(const std::string& certFile) { certFile_ = certFile; }

    /**
     * @brief 设置私钥文件路径。
     *
     * @param keyFile 私钥文件的路径字符串。
     */
    void setPrivateKeyFile(const std::string& keyFile) { keyFile_ = keyFile; }

    /**
     * @brief 设置证书链文件路径。
     *
     * @param chainFile 证书链文件的路径字符串。
     */
    void setCertificateChainFile(const std::string& chainFile) { chainFile_ = chainFile; }
    
    /**
     * @brief 设置 SSL/TLS 协议版本。
     *
     * @param version 要使用的 SSL/TLS 协议版本枚举值。
     */
    void setProtocolVersion(SSLVersion version) { version_ = version; }

    /**
     * @brief 设置允许的加密套件列表。
     *
     * @param cipherList 加密套件列表字符串，格式通常遵循 OpenSSL 规范。
     */
    void setCipherList(const std::string& cipherList) { cipherList_ = cipherList; }
    
    /**
     * @brief 设置是否启用客户端证书验证。
     *
     * @param verify 如果为 true，则要求并验证客户端证书；否则不验证。
     */
    void setVerifyClient(bool verify) { verifyClient_ = verify; }

    /**
     * @brief 设置客户端证书验证的最大深度。
     *
     * @param depth 验证链的最大深度。
     */
    void setVerifyDepth(int depth) { verifyDepth_ = depth; }
    
    /**
     * @brief 设置 SSL 会话的超时时间。
     *
     * @param seconds 会话超时时间，单位为秒。
     */
    void setSessionTimeout(int seconds) { sessionTimeout_ = seconds; }

    /**
     * @brief 设置 SSL 会话缓存的大小。
     *
     * @param size 会话缓存中可存储的最大会话数量。
     */
    void setSessionCacheSize(long size) { sessionCacheSize_ = size; }

    // Getters
    const std::string& getCertificateFile() const { return certFile_; }
    const std::string& getPrivateKeyFile() const { return keyFile_; }
    const std::string& getCertificateChainFile() const { return chainFile_; }
    SSLVersion getProtocolVersion() const { return version_; }
    const std::string& getCipherList() const { return cipherList_; }
    bool getVerifyClient() const { return verifyClient_; }
    int getVerifyDepth() const { return verifyDepth_; }
    int getSessionTimeout() const { return sessionTimeout_; }
    long getSessionCacheSize() const { return sessionCacheSize_; }

private:
    std::string certFile_; // 证书文件
    std::string keyFile_; // 私钥文件
    std::string chainFile_; // 证书链文件
    SSLVersion  version_; // 协议版本
    std::string cipherList_; // 加密套件
    bool        verifyClient_; // 是否验证客户端
    int         verifyDepth_; // 验证深度
    int         sessionTimeout_; // 会话超时时间
    long        sessionCacheSize_; // 会话缓存大小
};

} // namespace ssl