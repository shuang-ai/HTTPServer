#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <muduo/base/Logging.h>

/**
 * @brief 文件工具类，用于以二进制模式读取指定文件的内容。
 */
class FileUtil
{
public:
    FileUtil(std::string filePath)
        : filePath_(filePath)
        , file_(filePath, std::ios::binary) // 打开文件，二进制模式
    {}

    ~FileUtil()
    {
        file_.close();
    }

    /**
     * @brief 判断文件是否成功打开且有效。
     * @return 如果文件已打开则返回 true，否则返回 false。
     */
    bool isValid() const
    { return file_.is_open(); }
    
    /**
     * @brief 重置文件流并打开默认的“未找到”HTML资源文件。
     *
     * 该函数首先关闭当前已打开的文件流，然后以二进制模式重新打开
     * 位于指定路径的 NotFound.html 文件，通常用于HTTP 404响应。
     *
     * @param 无
     * @return 无
     */
    void resetDefaultFile()
    {
        file_.close();
        file_.open("/Gomoku/GomokuServer/resource/NotFound.html", std::ios::binary);
    }

    /**
     * @brief 获取当前关联文件的大小。
     *
     * 该函数通过将文件指针移动到末尾来获取文件大小，
     * 并在返回前将文件指针重置到开头，以确保不影响后续的读写操作。
     *
     * @return uint64_t 文件的大小（以字节为单位）。
     */
    uint64_t size()
    {
        file_.seekg(0, std::ios::end); // 定位到文件末尾
        uint64_t fileSize = file_.tellg();
        file_.seekg(0, std::ios::beg); // 返回到文件开头
        return fileSize;
    }
    
    /**
     * @brief 读取文件内容到缓冲区。
     *
     * 该函数将文件内容读取到指定的字符向量中。
     *
     * @param buffer 用于存储文件内容的字符向量。
     */
    void readFile(std::vector<char>& buffer)
    {
        // 这个buffer.data()表示第一个元素的指针
        if (file_.read(buffer.data(), size()))
        {
            LOG_INFO << "File content load into memory (" << size() << " bytes)";
        }    
        else
        {
            LOG_ERROR << "File read failed";
        }
    }

private:
    std::string     filePath_;
    std::ifstream   file_;
};