#include "sqlconnpool.h"


SqlConnPool::SqlConnPool(): useCount_(0), freeCount_(0) 
{ }

void SqlConnPool::Init(const char* host, int port, 
              const char* user, const char* pwd,
              const char* dbName, int connSize) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL* sql_h = nullptr;
        sql_h = mysql_init(sql_h);
        if (!sql_h) {
            // LOG_ERROR("MySql init error");
            assert(sql_h);
        }

        sql_h = mysql_real_connect(sql_h, host, user,
                                   pwd, dbName, port, nullptr, 0);
        
        if (!sql_h) {
            // LOG_ERROR("MySql connect error");
        }

        connQue_.push(sql_h);
    }
    MAX_CONN_ = connSize;

    // 线程间通信的信号量
    // 初始化为 MAX_CONN_
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL* sql_h = nullptr;
    if (connQue_.empty()) {
        // LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_); // 等待 semId_ > 0

    {
        std::lock_guard<std::mutex> lk(mtx_);
        sql_h = connQue_.front();
        connQue_.pop();
    }

    return sql_h;
}

void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    {
        std::lock_guard<std::mutex> lk(mtx_);
        // 重新入队
        connQue_.push(conn);
        sem_post(&semId_); // semId_ + 1
    }
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> lk(mtx_);
    return connQue_.size();
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> lk(mtx_);
    while (!connQue_.empty()) {
        MYSQL* sql_h = connQue_.front();
        connQue_.pop();
        mysql_close(sql_h);
    }
    mysql_library_end();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}