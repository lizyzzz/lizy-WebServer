/*
    组织响应报文
*/


#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <string>
#include <sys/stat.h>         // stat
#include <fcntl.h>            // open
#include <unistd.h>           // close
#include <sys/mman.h>         // mmap, munmap
#include "../buffer/buffer.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    /// @brief 初始化
    /// @param srcDir 资源文件夹路径
    /// @param path 文件路径
    /// @param isKeepAlive isKeepAlive
    /// @param code 状态码
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);

    /// @brief 组织响应报文
    /// @param buff 组织报文的结果
    void MakeResponse(Buffer& buff);

    /// @brief 释放虚拟地址
    void UnmapFile();

    /// @brief 返回文件映射到虚拟空间的地址
    /// @return 首地址
    char* File();

    /// @brief 获得文件长度
    /// @return 长度
    size_t FileLen() const;

    /// @brief 组织错误的文本信息
    /// @param buff 拼接后的结果
    /// @param message 附加信息
    void ErrorContent(Buffer& buff, std::string message);

    /// @brief 返回状态码
    /// @return 状态码
    int Code() const {
        return code_;
    }

private:
    /// @brief 组织响应报文状态行
    /// @param buff 拼接后的结果
    void AddStateLine_(Buffer& buff);
    /// @brief 组织响应报文首部行
    /// @param buff 拼接后的结果
    void AddHeader_(Buffer& buff);
    /// @brief 组织响应报文的内容行
    /// @param buff 拼接后的结果
    void AddContent_(Buffer& buff);

    /// @brief 当错误码发生时给出错误页面
    void ErrorHtml_();
    /// @brief 将请求的文件类型转化为 Content-type
    /// @return Content-type
    std::string GetFileType_();

private:
    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};




#endif