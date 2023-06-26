# lizy-WebServer
一个用C++14实现的高并发http服务器
## 1. 功能
* 利用Epoll边缘触发模式+线程池实现的单Reactor模型；
* 利用正则表达式和有限状态机解析HTTP请求报文；
* 使用mmap把响应的html文件映射到虚拟内存空间，加快传输速度；
* 用vector<char>封装空间可自动增长的字符串缓冲区；
* 基于最小堆实现定时器，管理长连接，配合epoll_wait()函数的超时参数处理超时连接；
* 利用单例模式+阻塞队列实现异步日志系统，满足不同等级的日志记录需求；
* 利用RAII机制实现数据库连接池，避免数据库连接对象过多，同时实现注册和登录功能。
## 2. 环境要求
* Linux
* C++14
* MySql

## 3. 目录树
```
.
├── src         源代码
│   ├── buffer    可自动增长的缓冲区
│   ├── http      http请求和响应
│   ├── log       异步日志系统
│   ├── timer     定时器
│   ├── pool      线程池，数据库连接池
│   ├── server    epoll, server封装
│   └── main.cpp
├── resources      静态资源
│   ├── index.html
│   ├── image
│   ├── video
│   ├── js
│   └── css
├── bin            可执行文件
│   └── server
├── logFile        日志文件
├── webbench-1.5   压力测试
├── build          
│   └── Makefile
├── Makefile
└── readme.md
```
## 4. 快速使用
* 编译
```
cd 当前目录
make
```
* 运行
```
cd 当前目录
./bin/server
```
* 测试
```
打开服务器输入: http://你的ip地址:端口号/
例如: http://127.0.0.1:8888/
```
## 5. 压力测试  
Ubuntu 20.04  
i5第11代  
内存: 8G
![image-webbench](https://github.com/lizyzzz/lizy-WebServer/blob/main/%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95.png)

## 6. 致谢
Linux高性能服务器编程，游双著.

[@markparticle](https://github.com/markparticle/WebServer)
