#include "clientsession.h"
#include <unordered_map>
#include "Log.h"
#include "epoll.h"

class ChatServer
{
private:

    Epoll epoll;
    UniqueFd listenfd;
    int port;
    bool running;

    std::unordered_map<int,ClientSession>clients;
public:

    explicit ChatServer(int max_events,int port_);
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
    
    void handleReadBuffer(int fd);
    void handleOneMessage(int fd, const std::string& msg);

    void closeClient(int fd);


};