/*
    webserver 实现
*/

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <errno.h>
#include <memory>
#include <mutex>

#include "epoller.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
// #include "../pool/ThreadPool.hpp"
// #include "../pool/threadpool.h"
#include "../http/httpconn.h"
#include "../../lizy_log/include/logging.h"
#include "../../lizy_timewheel/include/timewheel.h"

class ThreadPool;

class WebServer {
public:
    /// @brief 构造函数
    /// @param port 服务端口号
    /// @param trigMode epoll 触发模式 0-水平触发 1-连接边缘触发 2-监听边缘触发 3-连接和监听都是边缘触发(默认)
    /// @param timeoutMS 超时时间(单位:ms)
    /// @param OpenLinger 是否优雅关闭连接
    /// @param sqlPort 数据库端口号
    /// @param sqlUser 数据库用户名
    /// @param sqlPwd 数据库密码
    /// @param dbName 数据库库名
    /// @param sqlPoolNum 数据库连接池数量
    /// @param threadNum 线程池数量
    /// @param MaxEvent 最大同时发生的事件数
    WebServer(int port, int trigMode, int timeoutMS, bool OpenLinger,
              int sqlPort, const char* sqlUser, const char* sqlPwd,
              const char* dbName, int sqlPoolNum, int threadNum,
              int MaxEvent);
    
    ~WebServer();
    /// @brief 服务器运行函数
    void Start();
private:
    typedef std::shared_ptr<HttpConn> connPtr; // 使用智能指针

    /// @brief 初始化监听的 socket
    /// @return true-成功, false-失败
    bool InitSocket_();
    /// @brief 设置触发模式
    /// @param trigMode 0-水平触发 1-连接边缘触发 2-监听边缘触发 3-连接和监听都是边缘触发(默认)
    void InitEventMode_(int trigMode);
    /// @brief 添加连接上的客户端
    /// @param fd socketFd
    /// @param addr 通讯信息结构体
    void AddClient_(int fd, sockaddr_in addr);

    /// @brief 处理 accept 的业务
    void DealListen_();
    /// @brief 处理 写 业务
    /// @param client 客户端指针
    void DealWrite_(connPtr client);
    /// @brief 处理 读 业务
    /// @param client 客户端指针
    void DealRead_(connPtr client);

    /// @brief 发送错误信息并关闭连接
    /// @param fd socket fd
    /// @param info message
    void SendError_(int fd, const char* info);

    /// @brief 关闭客户端连接
    /// @param client 客户端结构体指针
    void CloseConn_(connPtr client);

    /// @brief 延长客户端超时时间
    /// @param client 客户端结构体指针
    void ExtentTime_(connPtr client);
    
    /// @brief 读数据函数
    /// @param client 客户端指针
    void OnRead_(connPtr client);
    /// @brief 写数据函数
    /// @param client 客户端指针
    void OnWrite_(connPtr client);
    void OnProcess(connPtr client);

    /// @brief 设置非阻塞方式
    /// @param fd 文件描述符
    /// @return 0-成功, -1-错误
    static int SetFdNonBlock(int fd);

private:
    static const int MAX_FD = 65536;
    
    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<TimeWheel> timeWheel_;   // 线程安全
    std::unique_ptr<ThreadPool> threadpool_; // 线程安全
    std::unique_ptr<Epoller> epoller_;       // 注意并发安全
    std::unordered_map<int, connPtr> users_; // fd-connPtr map(注意并发安全)

    std::mutex users_lock_;
};




#endif