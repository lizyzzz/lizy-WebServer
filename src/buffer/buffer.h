/*
    用 vector<char> 实现可自动增长的缓冲区
*/


#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <vector>
#include <string>
#include <atomic>
#include <assert.h>
#include <sys/uio.h> // readv
#include <unistd.h> // write

class Buffer {
public:
    Buffer(int BufferSize = 1024);
    ~Buffer() = default;

    /// @brief 返回剩余可写入的字节数
    /// @return 字节数大小
    size_t WritableBytes() const;
    /// @brief 返回可读取的字节数
    /// @return 字节数大小
    size_t ReadableBytes() const;
    /// @brief 返回读取的起始位置
    /// @return 起始位置
    size_t PrependableBytes() const;

    /// @brief 返回当前读取的位置
    /// @return 读取位置的地址
    const char* Peek() const;
    /// @brief 保证有足够的写入空间
    /// @param len 指定的写入空间
    void EnsureWritable(size_t len);
    /// @brief 更新已写入的字节数
    /// @param len 字节数长度
    void HasWritten(size_t len);

    /// @brief 恢复 len 长度(已读取过len长度)
    /// @param len 要恢复的长度
    void Retrieve(size_t len);
    /// @brief 恢复长度直到 end 指定的地址
    /// @param end 尾地址
    void RetrieveUntil(const char* end);
    /// @brief 恢复整个空间(置零)
    void RetrieveAll();
    /// @brief 恢复整个空间(置零)并把剩余的可读字符返回 string
    /// @return 返回的字符串
    std::string RetrieveAllToStr();
    /// @brief 返回写入位置的地址
    /// @return const 地址
    const char* BeginWriteConst() const;
    char* BeginWrite();

    /// @brief 添加字符数据
    /// @param str 字符串
    /// @param len 长度
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const std::string& str);
    void Append(const Buffer& buff);

    /// @brief 封装的 readv 函数(同时读到多个缓冲区)
    /// @param fd 文件描述符
    /// @param Errno 发生错误时的错误码
    /// @return 返回读取的长度
    ssize_t ReadFd(int fd, int* Errno);
    /// @brief 封装的 write 函数
    /// @param fd 文件描述符
    /// @param Errno 发生错误时的错误码
    /// @return 返回写入的长度
    ssize_t WriteFd(int fd, int* Errno);

private:
    /// @brief 返回 buffer 首地址指针
    /// @return 首地址指针
    char* BeginPtr_();
    /// @brief 返回 buffer 首地址常指针
    /// @return 首地址常指针
    const char* BeginPtr_() const;
    /// @brief 申请新的空间
    /// @param len 申请的长度
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};

#endif