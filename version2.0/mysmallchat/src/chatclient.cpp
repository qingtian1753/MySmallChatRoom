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
        if(states.out_queue.empty() && states.write_buffer.empty())
        {
            epoll.modEpollToRead(fd);
            return true;
        }

        if(states.write_buffer.empty() && !states.out_queue.empty())
        {
            states.write_buffer = states.out_queue.front();
            states.out_queue.pop();
        }

        while(states.write_pos < states.write_buffer.length())
        {
            ssize_t bytes_send = send(fd,states.write_buffer.data()+states.write_pos,
                                    states.write_buffer.length()-states.write_pos,0);
                
            if(bytes_send > 0)
            {
                states.write_pos += static_cast<size_t>(bytes_send);
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
bool ChatClient::queue_message_and_try_send( std::string& msg)
{
    std::string message = protocol::pack_message(msg);
    states.out_queue.emplace(message);
    return try_send(states.fd.getfd());
}

void ChatClient::clear_cur_line()
{
    std::cout<<"\r\033[K"<<std::flush;
}
void ChatClient::redraw_input_line()
{
    std::cout<<">"<<states.input_buffer<<std::flush;
}

void ChatClient::handle_one_message(std::string& msg)
{
    //'\n'由服务端发送，客户端只用输出msg就好
    std::cout<<msg;
}
void ChatClient::handle_read_buffer()
{
    while(1)
    {
        std::string msg; 
        if(!protocol::tryParseOne(states.read_buffer,msg))
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
            states.read_buffer.append(buffer,bytes_read);
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
    redraw_input_line();
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
                    if(!states.input_buffer.empty())
                    {   
                        if(!queue_message_and_try_send(states.input_buffer))
                        {
                            LOG_ERROR("send error");
                            return false;
                        }
                         states.input_buffer.clear();
                    }
                    redraw_input_line();
                }
                //删除键
                else if(ch == 127 || ch ==8)
                {
                    if(!states.input_buffer.empty())
                    {
                        states.input_buffer.pop_back();
                        clear_cur_line();
                        redraw_input_line();
                    }
                }   
                else if (ch >=32 && ch <=127)
                {
                    states.input_buffer.push_back(ch);
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

    if(!epoll.addFd(STDIN_FILENO))
    {
        LOG_ERROR("STDIN_FILENO can not be added to epoll");
        return false;
    }
    
    states.fd=std::move(server_fd);
    return true;
}

void ChatClient::run()
{
    redraw_input_line();
    while(1)
    {
        std::vector<epoll_event>evs(std::move(epoll.wait()));
        for(auto& ev : evs)
        {
            int fd=ev.data.fd;
            uint32_t ev_ = ev.events;
            if(fd==states.fd.getfd())
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
    }
}

ChatClient::ChatClient(int MAX_EVENTS):epoll(MAX_EVENTS)
{
        //更改终端的属性
        //保证按一次收一个，不用回车才接收信息
        // 获取终端属性
        tcgetattr(STDIN_FILENO,&orig_termios);

        struct termios raw = orig_termios;

        raw.c_lflag &= ~(ICANON | ECHO);

        raw.c_cc[VMIN]=1;
        raw.c_cc[VTIME]=0;

        tcsetattr(STDIN_FILENO,TCSANOW,&raw);

        //输入设置非阻塞
        int flags=fcntl(STDIN_FILENO,F_GETFL,0);
        fcntl(STDIN_FILENO,F_SETFL, flags | O_NONBLOCK);

}
ChatClient::~ChatClient()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&orig_termios);
}