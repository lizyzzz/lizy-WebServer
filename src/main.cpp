#include "server/webserver.h"
#include <signal.h>

int main(int argc, char const *argv[])
{

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
    /// @param openLog 日志开关
    /// @param logLevel 日志等级 0-debug, 1-info, 2-warn, 3-error, default-info
    /// @param logQueSize 日志异步队列长度
    /// @param MaxEvent 最大同时发生的事件数
    WebServer server(8888, 3, 60000, false,                   /*端口 ET模式 timeoutMs 优雅退出*/
                    3306, "root", "123456", "webserver",       /* mysql 配置 */
                    12, 6, true, 1, 1024, 1024);               /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列长度 最大同时发生的事件数*/

    server.Start();
    return 0;
}
