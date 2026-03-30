#include "chatserver.h"
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>

#include "socketutil.h"
#include "protocol.h"

ChatServer::ChatServer(int port_):port(port_),running(true)
{
    
}

ChatServer::~ChatServer()
{

}

bool ChatServer::init()
{
    if(!createListenSocket() || !createEpoll() )
    {
        return false;
    }
    SocketUtil::inepoll(epfd,listenfd);
    return true;
}

void ChatServer::run()
{
    epoll_event events[1024];
    while(running)
    {
        int n = epoll_wait(epfd,events,1024,-1);
        if(n<0)
        {
            if(errno == EINTR)
                continue;
            
            break;
        }

        for(int i=0;i<n;++i)
        {
            int fd=events[i].data.fd;
            uint32_t ev = events[i].events;
            //处理新连接
            if(fd == listenfd)
            {
                handleAccept();
                continue;
            }

            //处理可读事件
            if(ev & EPOLLIN)
            {
                handleReadEvent(fd);
            }

            //前面处理可读事件有可能遇到/quit，就会删除fd，所以要判断
            if(clients.find(fd) == clients.end())
            {
                continue;
            }
            
            //处理可写事件
            if(ev & EPOLLOUT)
            {
                handleWriteEvent(fd);
            }
        }
    }
}



bool ChatServer::createListenSocket()
{
    //set listenfd port reused
    int yes=1;
    
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))==-1) return false;

    //set port reused
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    struct sockaddr_in sa;
    memset(&sa,0,sizeof(sa));//initialize

    sa.sin_port=htons(port);
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=htonl(INADDR_ANY);//standard

    if(bind(listenfd,(sockaddr*)&sa,sizeof(sa))==-1 ||
       listen(listenfd,511)==-1)
       {
            close(listenfd);
            return false;
       }

    if(SocketUtil::setsocketnonblocknodelay(listenfd) == -1)
            return false;
       
    return true;
}

bool ChatServer::createEpoll()
{
    epfd = epoll_create1(0);

    return epfd==-1 ? false:true;
}
void ChatServer::handleAccept()
{
    while(1)
    {
        int client_fd = accept(listenfd,nullptr,nullptr);
        if(client_fd<0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                close(client_fd);
                std::cerr<<"accept error!"<<std::endl;
                return ;
            }
        }
        
        if(SocketUtil::setsocketnonblocknodelay(client_fd)==-1)
            return ;
        
        if(!SocketUtil::inepoll(epfd,client_fd))
        {
            close(client_fd);
            std::cerr<<"client can not in epoll!"<<std::endl;
            return ;
        }
        
        std::string welcome("Welcome to simple chat  "
        "use /name your_name to register your name\n");
        
        std::cout<<"new client!"<<std::endl;
        clients[client_fd].name="qingtian";
        queueMessage(client_fd,welcome);
    }
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
    while(1)
    {
        char buffer[4096]{};
        ssize_t bytes_read =read(fd,buffer,sizeof(buffer));
        if(bytes_read>0)
        {
            clients[fd].read_buffer.append(buffer,bytes_read);
            continue;
        }
        if(bytes_read == 0)
        {
            closeClient(fd);
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

        closeClient(fd);
        return ;
    }

    handleReadBuffer(fd);
}

void ChatServer::flushOneClient(int fd)
{
    while(1)
    {
        if(clients[fd].out_queue.empty() && clients[fd].write_buffer.empty())
        {
            SocketUtil::modEpollToRead(epfd,fd);
            return;
        }

        
        if(clients[fd].write_buffer.empty() && !clients[fd].out_queue.empty())
        {
            clients[fd].write_buffer = clients[fd].out_queue.front();
            clients[fd].out_queue.pop_front();
        }

        std::string& msg = clients[fd].write_buffer;
        size_t& pos = clients[fd].write_pos;
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

            closeClient(fd);
            return ;
        }
        clients[fd].write_clear();
    }
}

void ChatServer::closeClient(int fd)
{
    if(clients.find(fd) == clients.end())
    {
        return ;
    }

    close(fd);
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
    clients.erase(fd);
}

//拆包，拆完后交给handleOneMessage处理
void ChatServer::handleReadBuffer(int fd)
{
    while(1)
    {
        std::string msg; 
        if(!protocol::tryParseOne(clients[fd].read_buffer,msg))
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
        handleCommand(fd,msg);
    else
        handleBroadcast(fd,msg);
}

void ChatServer::handleCommand(int fd,const std::string& msg)
{
    //处理创建名称
    if(msg.find("/name ")==0)
    {
        //判断名字是否合法
        bool illegal = true;

        //第一个不是空格的元素的下标
        int pos1;
        //判断name后面是不是全是空格
        for(int i=6;i<msg.length();++i)
        {
            if(msg[i]!=' ' && illegal)
            {
                pos1 = i;
                illegal = false;
                break;
            }
        }

        //如果非法
        if(illegal)
        {
            std::string message = "the name is illegal!\n";
            queueMessage(fd,message);
            return ;
        }

        std::string new_name = msg.substr(pos1,msg.length()-pos1);

        //判断名字是否存在
        for(auto& it : clients)
        {
            if(it.second.name == new_name)
            {
                std::string message = "the name is already existing!\n";
                queueMessage(fd,message);
                return ;
            }
        }

        clients[fd].name = new_name;
        std::string message = new_name + " registered!\n";
        for(auto& it : clients)
        {
            queueMessage(it.first,message);
        }
        return ;
    }
    
    if(msg.find("/list") == 0 && msg.length() == 5)
    {
        std::string message("");
        for(auto& it : clients)
        {
            message+=it.second.name + "\n";
        }
        queueMessage(fd,message);
        return ;
    }

    if(msg.find("/id")== 0 && msg.length() ==3)
    {
        std::string message = clients[fd].name + "\n";
        queueMessage(fd,message);
        return ;
    }

    if(msg.find("/quit")== 0 && msg.length() == 5)
    {
        std::string message = clients[fd].name + " quit!\n";
        closeClient(fd);
        broadcastSystemMessage(message);
    }

    if(msg.find("/msg ") == 0)
    {
        int pos1 = msg.find(" ",5);
        std::string target_name = msg.substr(5,pos1-5);
        for(auto& it : clients)
        {
            //如果找到了目标，则进行私聊
            if(it.second.name == target_name)
            {
                std::string message = "[private]<"+clients[fd].name+">"+msg.substr(pos1+1,msg.length()-pos1-1) + "\n";
                queueMessage(it.first,message);

                message = "[private]<you to "+it.second.name+">"+msg.substr(pos1+1,msg.length()-pos1-1)+ "\n";
                queueMessage(fd,message);
                return ;
            }
        }
        std::string message = "the client is not existing!\n";
        queueMessage(fd,message);
        return ;
    }

    std::string message = "[system]illegal message!\n";
    queueMessage(fd,message);
}

void ChatServer::handleBroadcast(int fd, const std::string msg)
{
    std::string message = clients[fd].name+": "+msg +"\n";
    for(auto& it : clients)
    {
        queueMessage(it.first,message);
    }
}

//将消息入队并设置EPOLLOUT
void ChatServer::queueMessage(int fd,const std::string& msg)
{
    clients[fd].out_queue.emplace_back(protocol::pack_message(msg));
    SocketUtil::modEpollToWrite(epfd,fd);
}

void ChatServer::broadcastSystemMessage(const std::string& msg)
{
    std::string message = "[system]" + msg + "\n";
    for(auto& it : clients)
    {
        queueMessage(it.first,message);
    }
}
