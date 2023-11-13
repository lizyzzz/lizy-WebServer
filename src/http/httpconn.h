/*
    维护 http 连接

*/

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <arpa/inet.h>     // sockaddr_in
#include <atomic>
#include <string>
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "../../lizy_log/include/logging.h"
#include "httprequest.h"
#include "httpresponse.h"


class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    /// @brief 初始化函数
    /// @param sockFd socket 文件描述符
    /// @param addr 通信信息结构体
    void Init(int sockFd, const sockaddr_in& addr);

    /// @brief 从 socket 读取数据
    /// @param saveErrno 出错时 保存的错误码
    /// @return 读取的长度
    ssize_t Read(int* saveErrno);

    /// @brief 往 socketFd 写数据
    /// @param saveErrno 出错时 保存的错误码
    /// @return 写入的长度
    ssize_t Write(int* saveErrno);

    /// @brief 关闭连接, 资源回收
    void Close();

    /// @brief 返回 socket 描述符
    /// @return socketFd
    int GetFd() const;

    /// @brief 返回TCP连接的端口号
    /// @return 端口号
    int GetPort() const;

    /// @brief 返回 TCP连接 的 IP地址
    /// @return IP地址
    const char* GetIP() const;

    /// @brief 获取通讯信息结构体
    /// @return sockaddr_in
    sockaddr_in GetAddr() const;

    /// @brief request_解析请求报文, 并准备 response_ 对象(组织响应报文)
    /// @return true-成功, false-失败
    bool Process();

    /// @brief 剩余还没写入的字节数
    /// @return 字节数
    int ToWriteBytes();

    /// @brief 是否是 keepAlive
    /// @return true-Yes, false-No
    bool IsKeepAlive() const;

    /// @brief 获取conn对像超时任务的 key
    /// @return key
    std::string GetTimeOutKey() const;

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;
    std::string timeOutKey;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_;  // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};




#endif