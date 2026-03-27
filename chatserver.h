#include "clientsession.h"
#include <unordered_map>

class ChatServer
{
private:

    int epfd;
    int listenfd;
    int port;
    bool running;

    std::unordered_map<int,ClientSession>clients;
public:

    explicit ChatServer(int port_);
    ~ChatServer();

    //创建监听socket,设置非阻塞，创建epoll并添加监听socket
    bool init();

    void run();

private:
    //创建监听socket
    bool createListenSocket();
    //创建epoll
    bool createEpoll();

    void handleAccept();
    void handleReadEvent(int fd);
    void handleWriteEvent(int fd);

    void recvAll(int fd);
    void flushOneClient(int fd);
    void closeClient(int fd);
    
    void handleReadBuffer(int fd);
    void handleOneMessage(int fd, const std::string& msg);
    void handleCommand(int fd,const std::string& msg);
    void handleBroadcast(int fd, const std::string msg);

    void queueMessage(int fd, const std::string& msg);
    void broadcastSystemMessage(const std::string& msg);



};