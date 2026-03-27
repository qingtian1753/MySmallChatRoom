#include "chatclient.h"
#include "protocol.h"
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

bool ChatClient::try_send(int epfd,int fd)
{
    while(1)
    {
          //如果没有要发送的数据了就return
        if(states.out_queue.empty() && states.write_buffer.empty())
        {
            SocketUtil::modEpollToRead(epfd,fd);
            return true;
        }

        if(states.write_buffer.empty() && !states.out_queue.empty())
        {
            states.write_buffer = states.out_queue.front();
            states.out_queue.pop_front();
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
                SocketUtil::modEpollToWrite(epfd,fd);
                return true;
            }
            if(errno == EPIPE || errno == ECONNRESET)
            {
                close(fd);
                std::cout<<"server connection interrupt"<<std::endl;
                return false;
            }
            
            close(fd);
            std::cerr<<"try_send error!"<<std::endl;
            return false;
        }

        //清空缓冲区
        states.write_clear();
    }
}

//将消息加入到发送队列并尝试发送
bool ChatClient::queue_message_and_try_send(int epfd, std::string& msg)
{
    std::string message = protocol::pack_message(msg);
    states.out_queue.emplace_back(message);
    return try_send(epfd,states.fd);
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

int ChatClient::recv_all(const int fd)
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
            close(fd);
            return -1;
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

        close(fd);
        return -1;
    }

    handle_read_buffer();
    redraw_input_line();
    return 0;
}

bool ChatClient::handle_stdin(int epfd,int client_fd)
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
                    std::cerr<<"read stdin error\n";
                    close(epfd);
                    close(client_fd);
                    return false;
                }
            }
            else if( n == 0)
            {
                break;
            }
            else if( ch =='\r' || ch == '\n')
                {
                    clear_cur_line();
                    if(!states.input_buffer.empty())
                    {   
                        if(!queue_message_and_try_send(epfd,states.input_buffer))
                        {
                            std::cerr<<"send error\n";
                            close(epfd);
                            close(client_fd);
                            return false;
                        }
                         states.input_buffer.clear();
                    }
                    redraw_input_line();
                }
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
  int server_fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=inet_addr(ip);

    if(connect(server_fd,(sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        std::cerr<<" connected failed"<<std::endl;
        close(server_fd);
        return false;
    }

    states.fd=server_fd;
    SocketUtil::set_raw_mode(orig_termios);
    SocketUtil::set_stdin_nonblocking();
    SocketUtil::setsocketnonblocknodelay(server_fd);
    
    epfd=epoll_create1(0);
    if(SocketUtil::inepoll(epfd,server_fd)==-1)
    {
        std::cerr<<"client can not in epoll!\n";
        close(server_fd);
        return false;
    }

    if(SocketUtil::inepoll(epfd,STDIN_FILENO)==-1)
    {
        std::cerr<<"STDIN_FILENO can not be added to epoll\n";
        close(server_fd);
        return false;
    }

    return true;
}

void ChatClient::run()
{
    
    constexpr int MAX_EVENTS=10;
    epoll_event events[MAX_EVENTS];

    redraw_input_line();
    while(1)
    {
        int nfds=epoll_wait(epfd,events,MAX_EVENTS,-1);
        if(nfds==-1)
        {
            //interupted by signal
            if(errno == EINTR) continue;

            std::cerr<<"epoll_wait error\n";
            return ;
        }

            for(int i=0;i<nfds;++i)
            {
                if(events[i].data.fd==states.fd)
                {
                    uint32_t ev =events[i].events;
                    if(ev & EPOLLIN)
                    {
                        if(recv_all(states.fd) == -1)
                        {
                            std::cerr<<"recv_all error"<<std::endl;
                            close(states.fd);
                            close(epfd);
                            return ;
                        }
                    }

                    if(ev & EPOLLOUT)
                    {
                        if(!try_send(epfd,states.fd))
                        {
                            return ;
                        }
                    }
                }
                else if(events[i].data.fd == STDIN_FILENO)
                {
                    if(!handle_stdin(epfd,states.fd))
                    {
                        std::cerr<<"handle_stdin error!"<<std::endl;
                        return ;
                    }
                }
        }
    }

}



ChatClient::~ChatClient()
{
    SocketUtil::reset_terminal(orig_termios);
    close(states.fd);
    close(epfd);
}