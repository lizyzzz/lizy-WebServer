/*
    RAII机制 管理从数据库连接池获得的连接句柄
*/

#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"


class SqlConnRAII {
public:
    SqlConnRAII(SqlConnPool* connpool): sql_h_(nullptr) {
        assert(connpool);
        if (connpool) {
            sql_h_ = connpool->GetConn();
            connpool_ = connpool;
        }
    }

    ~SqlConnRAII() {
        if (sql_h_) {
            connpool_->FreeConn(sql_h_);
        }
    }
    
    bool HasPtr() {
        return sql_h_ != nullptr;
    }

    MYSQL* GetPtr() {
        return sql_h_;
    }


private:
    MYSQL* sql_h_;
    SqlConnPool* connpool_;

};


#endif
