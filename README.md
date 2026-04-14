# MySmallChatRoom

一个基于 C++11/14 和 epoll 的轻量级聊天室系统，包含服务端和命令行客户端。支持多客户端实时通信、私聊、心跳检测及自定义命令。

##  核心特性

- **高并发网络模型**：epoll (ET 模式) + 非阻塞 I/O，单线程支持数千并发连接。
- **自定义应用层协议**：长度头（4字节）+ 消息体，彻底解决 TCP 粘包/半包问题。
- **命令行客户端**：基于 termios 的 raw mode 实现单字符输入、实时回显、退格编辑。
- **心跳保活机制**：客户端定时发送 `/ping`，服务端响应 `/pong`，超时自动断开。
- **命令系统**：支持 `/name`、`/list`、`/msg`、`/quit`、`/help` 等指令，采用函数映射表解耦。
- **私聊与广播**：支持全局广播和点对点私聊，消息带有发送者标识。
- **RAII 资源管理**：文件描述符、epoll 实例自动释放，杜绝泄漏。

##  技术栈

- **语言**：C++11/14
- **网络**：epoll、非阻塞 I/O、Reactor 模型
- **终端**：termios (raw mode)
- **工具**：Make/G++、Linux 环境

##  项目结构
.
├── src/
│ ├── server.cpp # 服务端入口
│ ├── client.cpp # 客户端入口
│ ├── chatserver.cpp/.h # 服务端主逻辑
│ ├── chatclient.cpp/.h # 客户端主逻辑
│ ├── clientsession.cpp/.h # 服务端连接会话
│ ├── clientstate.cpp/.h # 客户端状态管理
│ ├── epoll.cpp/.h # epoll 封装
│ ├── handleclient.cpp/.h # 命令处理与消息分发
│ ├── protocol.h # 自定义协议（打包/解析）
│ ├── socketutil.cpp/.h # socket 辅助函数
│ ├── uniquefd.h # RAII 文件描述符
│ └── Log.cpp/.h # 简易日志
├── Makefile
└── README.md

text

##  快速开始

### 依赖
- Linux 环境 (epoll)
- g++ (支持 C++14)

### 编译

git clone https://github.com/qingtian1753/MySmallChatRoom.git
cd MySmallChatRoom
make server
make client
