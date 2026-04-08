#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "chatserver.h"
#include "socketutil.h"
#include "protocol.h"
#include "handleclient.h"
ChatServer::ChatServer(int max_events,int port_)
:port(port_),running(true)
,epoll(max_events)
{
    
}

ChatServer::~ChatServer()
{
    
}

bool ChatServer::init()
{
    if(!createListenSocket() || !epoll.addFd(listenfd.getfd()))
    {
        return false;
    }

    return true;
}

void ChatServer::run()
{
    while(running)
    {
        //每2s醒一次检查一下客户端连接情况
        std::vector<epoll_event>fds(std::move(epoll.wait(2000)));
        for(auto& ev : fds)
        {
            int fd=ev.data.fd;
            uint32_t ev_ = ev.events;
            //处理新连接
            if(fd == listenfd.getfd())
            {
                handleAccept();
                continue;
            }

            //处理可读事件
            if(ev_ & EPOLLIN)
            {
                handleReadEvent(fd);
            }

            //前面处理可读事件有可能遇到/quit，就会删除fd，所以要判断
            if(clients.find(fd) == clients.end())
            {
                continue;
            }
            
            //处理可写事件
            if(ev_ & EPOLLOUT)
            {
                handleWriteEvent(fd);
            }
        }
        checkTimeoutClients();
    }
}



bool ChatServer::createListenSocket()
{
    //set listenfd port reused
    int yes=1;
    int listen_fd;
    if((listen_fd=socket(AF_INET,SOCK_STREAM,0))==-1) return false;
    UniqueFd fd(listen_fd);

    //set port reused
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))==-1)
        return false;

    struct sockaddr_in sa{};

    sa.sin_port=htons(port);
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=htonl(INADDR_ANY);//standard

    if(bind(listen_fd,(sockaddr*)&sa,sizeof(sa))==-1 ||
       listen(listen_fd,520)==-1)
       {
            LOG_ERROR("chatserver bind or listen error!");
            return false;
       }
            

    if(!SocketUtil::setsocketnonblocknodelay(listen_fd))
    {
        LOG_ERROR("chatserver setsocketnonblocknodelay!");
        return false;
    }
            
    
    listenfd=std::move(fd);
    return true;
}

void ChatServer::handleAccept()
{
    while(1)
    {
        UniqueFd client_fd(accept(listenfd.getfd(),nullptr,nullptr));
        int key_fd = client_fd.getfd();
        if(key_fd<0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                LOG_ERROR("accept error!");
                return ;
            }
        }
        initClient(std::move(client_fd));
    }
}

void ChatServer::initClient(UniqueFd&& client_fd)
{
    if(!SocketUtil::setsocketnonblocknodelay(client_fd.getfd()))
    {
        LOG_ERROR("initClient error!");
        return ;
    }
        

    if(!epoll.addFd(client_fd.getfd()))
    {
        LOG_ERROR("client can not in epoll!");
        return ;
    }

    std::string welcome("Welcome to simple chat  "
    "use /name your_name to register your name");
        
    int key_fd=client_fd.getfd();                                  
    //利用移动语义将client_fd的所有权转移到临时的clientsession变量里
//emplace返回的是std::pair<iterator,bool>      //再用移动语义将cs的所有权转移给clients，禁止一切拷贝
    auto [it,inserted] = clients.emplace(key_fd,std::move(ClientSession(std::move(client_fd))));
    it->second.setName("qingtian");
    handle::queueMessage(key_fd,welcome,clients,epoll);
    LOG_DEBUG("new client! fd="+std::to_string(key_fd));
}
void ChatServer::handleReadEvent(int fd)
{
    recvAll(fd);
}

void ChatServer::handleWriteEvent(int fd)
{
    flushOneClient(fd);
}


void ChatServer::recvAll(int fd)
{
    //迭代器里包含一个指向哈希表桶数组的指针，用于遍历
    //还包含指向节点的指针，使用了*,->重载
    auto it = clients.find(fd);
    //fd只要不出什么bug一定存在，但最好还是判断一下
    if(it==clients.end())
        return ;

    while(1)
    {
        char buffer[4096]{};
        ssize_t bytes_read =read(fd,buffer,sizeof(buffer));
        if(bytes_read>0)
        {
            it->second.addReadBuffer(buffer,bytes_read);
            it->second.refreshActiveTime();
            continue;
        }
        if(bytes_read == 0)
        {
            handle::closeClient(fd,epoll,clients);
            LOG_INFO("clients fd="+std::to_string(fd)+"disconnected normally");
            return ;
        }

        if(errno == EINTR)
        {
            continue;
        }

        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break;
        }

        handle::closeClient(fd,epoll,clients);
        return ;
    }

    handleReadBuffer(fd);
}

void ChatServer::flushOneClient(int fd)
{

    auto it = clients.find(fd);
    if(it == clients.end())
        return ;
    while(1)
    {
        if(it->second.empty())
        {
            epoll.modEpollToRead(fd);
            return;
        }

        it->second.loadOneMessage();
        std::string& msg = it->second.getWriteBuffer();
        size_t& pos = it->second.getWritePos();
        size_t len = msg.length();
        while(pos< len)
        {
            ssize_t bytes_send = send(fd,msg.data()+pos,len-pos,0);

            if(bytes_send>0)
            {
                pos+=bytes_send;
                continue;
            }
            if(errno == EINTR)
            {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return ;
            }

            handle::closeClient(fd,epoll,clients);
            return ;
        }
        it->second.write_clear();
    }
}

//拆包，拆完后交给handleOneMessage处理
void ChatServer::handleReadBuffer(int fd)
{
    auto it =clients.find(fd);
    if( it == clients.end() )
        return ;

    while(1)
    {
        std::string msg; 
        if(!protocol::tryParseOne(it->second.getReadBuffer(),msg))
        {
           break;
        }
        handleOneMessage(fd,msg);
        //如果客户端输入/quit则会删除client_fd，之后就不能再访问了，所以要进行判断
        if(clients.find(fd) == clients.end())
           break;
    }
}
void ChatServer::handleOneMessage(int fd,const std::string& msg)
{
    if(msg.empty())
        return ;
    
    if(msg[0]=='/')
        handle::handleCommand(fd,msg,clients,epoll);
    else
        handle::handleBroadcast(fd,msg,clients,epoll);
}

void ChatServer::checkTimeoutClients()
{
    const int timeout_seconds = 20;
    auto now = std::chrono::steady_clock::now();
    std::vector<int>fds;
    for(auto& it : clients)
    {
        if(it.second.isTimeout(now,timeout_seconds))
        {
            fds.emplace_back(it.first);
        }
    }
    for(int& v : fds)
    {
            LOG_INFO("client fd="+std::to_string(v)+"timeout, closing connection!");
            handle::closeClient(v,epoll,clients);
            handle::broadcastSystemMessage("client fd="+std::to_string(v)+"timeout, closing connection!",clients,epoll);
    }
}