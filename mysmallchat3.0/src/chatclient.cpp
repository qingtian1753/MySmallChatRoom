#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <fcntl.h>

#include "Log.h"
#include "chatclient.h"
#include "protocol.h"
bool ChatClient::try_send(int fd)
{
    while(1)
    {
          //如果没有要发送的数据了就return
        if(states.empty())
        {
            epoll.modEpollToRead(fd);
            return true;
        }

        states.loadOneMessage();

        size_t& pos = states.getWritePos();
        std::string& write_buffer = states.getWriteBuffer();
        size_t len = write_buffer.length();
        while(pos < len)
        {
            ssize_t bytes_send = send(fd,write_buffer.data()+pos,
                                    write_buffer.length()-pos,0);
                
            if(bytes_send > 0)
            {
                pos += static_cast<size_t>(bytes_send);
                continue;
            }
            else if(errno == EINTR)
            {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {   
                epoll.modEpollToWrite(fd);
                return true;
            }
            if(errno == EPIPE || errno == ECONNRESET)
            {
                LOG_WARN("server connection interrupt");
                return false;
            }
            
            LOG_ERROR("try_send error!");
            return false;
        }

        //清空缓冲区
        states.write_clear();
    }
}

//将消息加入到发送队列并尝试发送
bool ChatClient::queue_message_and_try_send(std::string& msg)
{
    states.enqueueMsg(protocol::pack_message(msg));
    return try_send(states.getFd().getfd());
}

void ChatClient::clear_cur_line()
{
    //清空这一行，并且光标回到第一格
    std::cout<<"\r\033[K"<<std::flush;
}
void ChatClient::redraw_input_line()
{
    std::cout<<">"<<states.getInputBuffer()<<std::flush;
}

void ChatClient::handle_one_message(std::string& msg)
{
    //是/pong就不要打印，直接返回
    if(msg == "/pong\n")
    {
        return ;
    }
    clear_cur_line();
    //'\n'由服务端发送，客户端只用输出msg就好
    std::cout<<msg;
    redraw_input_line();
}
void ChatClient::handle_read_buffer()
{
    while(1)
    {
        std::string msg; 
        if(!protocol::tryParseOne(states.getReadBuffer(),msg))
        {
           break;
        }
        handle_one_message(msg);
    }
}

bool ChatClient::recv_all(const int fd)
{
    while(1)
    {
        char buffer[4096]={};
        memset(buffer,0,sizeof(buffer));
        ssize_t bytes_read=recv(fd,buffer,sizeof(buffer),0);
        if(bytes_read>0)
        {
            states.refreshActiveTime();
            states.addReadBuffer(buffer,bytes_read);
            continue;
        }
        else  if(bytes_read==0)
        {
            return false;
        } 

            //被信号中断了，可能是ctrl+c或者其他的信号，应该再试
        if(errno == EINTR)  
        {
            continue;
        }

            //资源暂时不可用
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break;
        }

        return false;
    }

    handle_read_buffer();
    return true;
}

bool ChatClient::handle_stdin(int client_fd)
{
    while(1)
        {
            char ch;
            ssize_t n = read(STDIN_FILENO,&ch,1);

            if( n == -1 )
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else 
                {
                    clear_cur_line();
                    LOG_ERROR("read stdin error");
                    return false;
                }
            }
            else if( n == 0)
            {
                break;
            }
            //敲下回车就发送
            else if( ch =='\r' || ch == '\n')
                {
                    clear_cur_line();
                    if(!states.getInputBuffer().empty())
                    {   
                        if(!queue_message_and_try_send(states.getInputBuffer()))
                        {
                            LOG_ERROR("send error");
                            return false;
                        }
                         states.inputClear();
                    }
                    redraw_input_line();
                }
                //删除键
                else if(ch == 127 || ch ==8)
                {
                    if(!states.getInputBuffer().empty())
                    {
                        states.getInputBuffer().pop_back();
                        clear_cur_line();
                        redraw_input_line();
                    }
                }   
                else if (ch >=32 && ch <=127)
                {
                    states.getInputBuffer().push_back(ch);
                    clear_cur_line();
                    redraw_input_line();
                }
        }
    return true;
}

bool ChatClient::initclient(const char* ip,int port)
{
    UniqueFd server_fd(socket(AF_INET,SOCK_STREAM,0));
    int fd = server_fd.getfd();
    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=inet_addr(ip);

    if(connect(fd,(sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        LOG_ERROR(" connected failed");
        return false;
    }

    if(!SocketUtil::setsocketnonblocknodelay(server_fd.getfd()))
    {
        LOG_ERROR("client setsocketnonblockingnodelay failed");
        return false;
    }
    
    if(!epoll.addFd(fd))
    {
        LOG_ERROR("client can not in epoll!");
        return false;
    }

    SocketUtil::setRawMode(orig_termios);
    SocketUtil::setStdinNonblocking();
    if(!epoll.addFd(STDIN_FILENO))
    {
        LOG_ERROR("STDIN_FILENO can not be added to epoll");
        return false;
    }
    
    states.getFd()=std::move(server_fd);
    return true;
}

void ChatClient::run()
{
    redraw_input_line();
    while(1)
    {
        std::vector<epoll_event>evs(std::move(epoll.wait(1000)));
        for(auto& ev : evs)
        {
            int fd=ev.data.fd;
            uint32_t ev_ = ev.events;
            if(fd==states.getFd().getfd())
            {
                
                if(ev_ & EPOLLIN)
                {
                    if(!recv_all(fd))
                    {
                        LOG_ERROR("recv_all error");
                        return ;
                    }
                }

                if(ev_ & EPOLLOUT)
                {
                    if(!try_send(fd))
                    {
                        LOG_ERROR("try_send failed!");
                        return ;
                    }
                }
            }
            else if(fd == STDIN_FILENO)
            {
                if(!handle_stdin(fd))
                {
                    LOG_ERROR("handle_stdin error!");
                    return ;
                }
            }
        }

        if(!checkTimeout())
            return ;
    }
}

ChatClient::ChatClient(int MAX_EVENTS):epoll(MAX_EVENTS)
{
        
    SocketUtil::getTerminal(orig_termios);
}
ChatClient::~ChatClient()
{
    SocketUtil::resetTerminal(orig_termios);
}

bool ChatClient::checkTimeout()
{
    //如果timeout_seconds时间内没有收到服务端消息那么就发送
    const int timeout_seconds = 10;
    auto now = std::chrono::steady_clock::now();
    //10s内没收到服务端信息 && waiting_pong是false就发/ping
    if(!states.getWaitingPong() && states.isTimeout(now,timeout_seconds))
        sendPing();
        
        //如果发完ping的五秒内没有收到pong
        //则服务端无响应了，关闭连接
    if(states.getWaitingPong() && states.isTimeout(now,15))
    {
        return false;
    }

    return true;
}
void ChatClient::sendPing()
{
    queue_message_and_try_send(states.getPingMsg());
    states.getWaitingPong()  = true;
}