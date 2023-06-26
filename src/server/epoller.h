/*
    epoll的封装
*/



#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>     // close
#include <cstring>
#include <vector>
#include <errno.h>
#include <assert.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    /// @brief 添加监听描述符
    /// @param fd 监听的描述符
    /// @param events 对应的事件(EPOLLIN, EPOLLOUT, EPOLLET等)
    /// @return true-成功, false-失败
    bool AddFd(int fd, uint32_t events);

    /// @brief 修改监听描述符的事件
    /// @param fd 监听的描述符
    /// @param events 对应的事件(EPOLLIN, EPOLLOUT, EPOLLET等)
    /// @return true-成功, false-失败
    bool ModFd(int fd, uint32_t events);

    /// @brief 删除监听描述符的事件
    /// @param fd 监听的描述符
    /// @return true-成功, false-失败
    bool DelFd(int fd);

    /// @brief 阻塞等待直到有事件发生
    /// @param timeout 超时时间 (-1表示无限等待)
    /// @return 发生的事件数量
    int Wait(int timeout = -1);

    /// @brief 返回位置 i 的事件的fd
    /// @param i 位置 i
    /// @return fd (错误返回-1)
    int GetEventFd(size_t i) const;

    /// @brief 返回位置 i 的事件的 events
    /// @param i 位置 i
    /// @return events (错误返回 0)
    uint32_t GetEvent(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};





#endif