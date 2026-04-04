#pragma once
#include <unistd.h>
#include "epoll.h"
class UniqueFd{

public:
    UniqueFd():fd(-1){}
    UniqueFd(int fd_):fd(fd_){}
    ~UniqueFd()
    {
        if(fd!=-1)
        close(fd);
    }
    //右值引用构造，之后移动语义构造用得到
    UniqueFd(UniqueFd&& other):fd(other.release()){}

    //删除拷贝构造保证fd的独占性
    //只允许移动
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&)=delete;


    UniqueFd& operator=(UniqueFd&& other)
    {
        if(this != &other)
        {
            //重置自己，并且关闭fd(如果可以关的话)
            reset();
            fd = other.release();
        }
        return *this;
    }

    void reset()
    {
        if(fd != -1)
        {
            close(fd);
            fd=-1;
        }
            
    }

    //返回fd
    int getfd() const
    {
        return fd;
    }

    bool valid() const
    {
        return fd == -1?false:true;
    }

    //释放掉fd的所有权并将fd返回给对方
    int release()
    {
        int temp = fd;
        fd = -1;
        return temp;
    }
    
private:
    int fd;

};