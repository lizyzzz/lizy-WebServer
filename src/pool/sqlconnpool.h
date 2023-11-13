/*
    数据库连接池
*/


#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <queue>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <assert.h>
#include "../../lizy_log/include/logging.h"

class SqlConnPool {
public:
    // 单例模式
    /// @brief 获取单例指针
    /// @return SqlConnPool指针
    static SqlConnPool* GetInstance() {
        static SqlConnPool inst;
        return &inst;
    }

    /// @brief 获得一个数据库连接句柄
    /// @return 数据库连接句柄指针
    MYSQL* GetConn();

    /// @brief 释放一个数据库连接句柄
    /// @param conn 要释放的句柄指针
    void FreeConn(MYSQL* conn);

    /// @brief 获得当前空闲的数据库连接句柄
    /// @return 空闲个数
    int GetFreeConnCount();

    /// @brief 初始化数据库连接池
    /// @param host 数据库主机名
    /// @param port 数据库端口, 一般为 3306
    /// @param user 数据库用户名
    /// @param pwd  数据库用户对应的密码
    /// @param dbName 要连接的数据库名
    /// @param connSize 连接数
    void Init(const char* host, int port, 
              const char* user, const char* pwd,
              const char* dbName, int connSize = 10);

    /// @brief 关闭数据库连接池
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

private:

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    std::queue<MYSQL*> connQue_;
    std::mutex mtx_;

    sem_t semId_;
};




#endif