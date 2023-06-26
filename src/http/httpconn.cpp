#include "httpconn.h"

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn(): fd_(-1), isClose_(true) {
    memset(&addr_, 0, sizeof(addr_));
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int sockFd, const sockaddr_in& addr) {
    assert(sockFd > 0);
    userCount++;
    addr_ = addr;
    fd_ = sockFd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), userCount.load());
}

void HttpConn::Close() {
    response_.UnmapFile();   // ******** 重点 ********
    if (isClose_ == false) {
        isClose_ = true;
        userCount--;
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d", fd_, GetIP(), GetPort(), userCount.load());
        close(fd_);
        fd_ = -1;  // 重新置为 -1
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

ssize_t HttpConn::Write(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 多缓冲区写
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }

        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            // 缓冲区字节被全部写完
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            // 第一个缓冲区被写完

            // 更新第二个缓冲区未被写的部分
            /* 第二缓冲区一般是 File 映射到虚拟空间的地址 */
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                // 清空第一缓冲区
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            // 第一缓冲区还没被写完
            iov_[0].iov_base = (uint8_t*) iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

ssize_t HttpConn::Read(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 一次过读取
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

int HttpConn::ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::IsKeepAlive() const {
    return request_.IsKeepAlive();
}

bool HttpConn::Process() {
    /*
    读-解析/准备-写:
        Read 函数在 Process 之前
        Write 函数在 Process 之前
    写:
        直接返回false
    */ 
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0) {
        // 没有读取到数据
        return false;
    }
    else if (request_.parse(readBuff_)) {
        LOG_DEBUG("Request path: %s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);
    /* 响应报文 状态行 首部行 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 响应的文件 */
    if (response_.File() && response_.FileLen() > 0) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }

    LOG_DEBUG("Response fileSize:%d, channel:%d, totalBytes:%d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
} 

