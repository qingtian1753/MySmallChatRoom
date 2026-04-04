#include "epoll.h"
#include "Log.h"



Epoll::Epoll(int MAX_EVENTS):max_events(MAX_EVENTS)
{
    epfd = epoll_create1(0);
    if(epfd < 0)
    LOG_ERROR("epoll create failed");
    events = new epoll_event[max_events];
}
Epoll::~Epoll()
{
    if(epfd != -1)
        close(epfd);

    delete []events;
}

std::vector<epoll_event>  Epoll::wait()
{
    std::vector<epoll_event>fds;
    int n=epoll_wait(epfd,events,max_events,-1);
    for(int i=0;i<n;++i)
    {
        fds.emplace_back(events[i]);
    }
    return fds;
}
bool Epoll::addFd(int fd)
{
    epoll_event ev{};
    ev.data.fd =fd;
    ev.events=EPOLLIN | EPOLLET;
    
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev)==-1)
    {
        LOG_ERROR("epoll addFd failed!");
        return false;
    }

    return true;    
}

int Epoll::getfd() const
{
    return epfd;
}

void Epoll::modEpollToRead(int fd)
{
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}
void Epoll::modEpollToWrite(int fd)
{
    epoll_event ev{};
    ev.data.fd= fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}

void Epoll::delFd(int fd)
{
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
}

    