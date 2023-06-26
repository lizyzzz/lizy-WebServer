#include "webserver.h"


WebServer::WebServer(int port, int trigMode, int timeoutMS, bool OpenLinger,
              int sqlPort, const char* sqlUser, const char* sqlPwd,
              const char* dbName, int sqlPoolNum, int threadNum,
              bool openLog, int logLevel, int logQueSize, int MaxEvent) :
              port_(port), openLinger_(OpenLinger), timeoutMS_(timeoutMS), isClose_(false),
              timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller(MaxEvent))
{
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);

    HttpConn::srcDir = srcDir_;
    HttpConn::userCount = 0;
    SqlConnPool::GetInstance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, sqlPoolNum);

    InitEventMode_(trigMode);
    if (!InitSocket_()) {
        isClose_ = true;
    }
    if (openLog) {
        Log::GetInstance()->init(logLevel, "./logFile", ".log", logQueSize);
        if (isClose_) {
            LOG_ERROR("========================= Server init error! =======================");
        }
        else {
            LOG_INFO("========================= Server init! =======================");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, openLinger_ ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                        (listenEvent_ & EPOLLET ? "ET" : "LT"),
                        (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", sqlPoolNum, threadNum);
        }
    }
    
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::GetInstance()->ClosePool();
}


void WebServer::Start() {
    int timeMS = -1; // epoll_wait timeout == -1 表示没有事件发生就阻塞
    if (!isClose_) {
        LOG_INFO("========================= Server Start! =======================");
    }
    while (!isClose_) {
        if (timeoutMS_ > 0) {
            /*
                处理超时事件
                并记录下一个超时事件的发生时间
            */ 
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS); // 最多等待下一次超时事件发生
        for (int i = 0; i < eventCnt; ++i) {
            /*  处理事件 */
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvent(i);
            if (fd == listenFd_) {
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                // CloseConn_(users_[id]); // 避免CloseConn_两次
                timer_->doWork(fd);
            }
            else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(users_[fd]);
            }
            else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(users_[fd]);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}



bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有的网卡地址
    addr.sin_port = htons(port_);

    struct linger optLinger;
    memset(&optLinger, 0, sizeof(optLinger));
    if (openLinger_) {
        /* 优雅关闭: 直到所剩的数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1; // 超时时间
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linget error!");
        return false;
    }

    // 端口号复用
    // 只有最后一个 socket 会正常接收数据
    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Set socket setsocketopt error!");
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Bind error!");
        return false;
    }

    ret = listen(listenFd_, 6);
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Listen error!");
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        close(listenFd_);
        LOG_ERROR("Add listenfd error!");
        return false;
    }
    
    SetFdNonBlock(listenFd_);
    return true;
}

int WebServer::SetFdNonBlock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::InitEventMode_(int trigMode) {
    /*
        EPOLLRDHUB:
        在内核2.6.17（不含）以前版本，
        要想知道对端是否关闭socket，上层应用只能通过调用recv来进行判断，
        在2.6.17以后，这种场景上层只需要判断EPOLLRDHUP即可，无需在调用recv这个系统调用。
    */
    listenEvent_ = EPOLLRDHUP; // 当客户端关闭连接时, 内核直接关闭, 不用再判断recv读取为0
    /*
        EPOLLONESHOT:
            为关联的文件描述符设置一次性行为。
            这意味着在使用 epoll_wait(2) 拉出事件后，关联的文件描述符在内部被禁用，
            并且 epoll 接口不会报告其他事件。
            用户必须使用 EPOLL_CTL_MOD 调用 epoll_ctl() 以使用新的事件掩码重新装备文件描述符。
    */
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;

    switch (trigMode) {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            connEvent_ |= EPOLLET;
            listenEvent_ |= EPOLLET;
            break;
        default:
            connEvent_ |= EPOLLET;
            listenEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}


void WebServer::SendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("Send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());

    client->Close();
    
    int fd = client->GetFd();
    delete client;
    client = nullptr;
    users_.erase(fd); // ***** 这里要把 fd 对应地从 users_ 删除 *****
}

void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if (timeoutMS_ > 0) {
        timer_->adjust(client->GetFd(), timeoutMS_);
    }
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);

    HttpConn* hc = new HttpConn();
    hc->Init(fd, addr);
    users_[fd] = hc;

    if (timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonBlock(fd);
    LOG_INFO("Client[%d] in!", users_[fd]->GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if (fd <= 0) {
            return;
        }
        else if (HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Client is full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET); //保证读完
}

void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client); // 延长时间
    threadpool_->enqueue(&WebServer::OnRead_, this, client); // 线程池处理
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client); // 延长时间
    threadpool_->enqueue(&WebServer::OnWrite_, this, client); // 线程池处理
}

void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->Read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn* client) {
    if (client->Process()) {
        // 读数据并成功解析请求报文, 准备发送响应报文
        // 所以变成 EPOLLOUT 
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else {
        // 写数据结束, 改为读 EPOLLIN
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);

    int ret = -1;
    int writeErrno = 0;
    ret = client->Write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        // 传输完成
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if (writeErrno == EAGAIN) {
            // 继续传输
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    // 其他情况
    CloseConn_(client);
}

