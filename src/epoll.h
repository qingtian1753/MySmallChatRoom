#pragma once
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
//有Epoll类来管理epoll的获取与释放   RAII

class Epoll
{

public:
    Epoll(int MAX_EVENTS=1024);
    ~Epoll();
    //禁止拷贝
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    int getfd() const;
    std::vector<epoll_event> wait();
    //将socketfd添加到epoll
    bool addFd(int fd);
    //切换socketfd的epoll_event模式
    void modEpollToRead(int fd);
    void modEpollToWrite(int fd);
    void delFd(int fd);
private:
    int epfd;
    epoll_event* events;
    int max_events;
};