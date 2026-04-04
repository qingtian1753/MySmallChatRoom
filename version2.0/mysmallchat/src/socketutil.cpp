#include "socketutil.h"
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <termios.h>

namespace SocketUtil{
    bool setsocketnonblocknodelay(int fd)
    {
        int flags,yes=1;

        //设置socket非阻塞
        if ((flags = fcntl(fd, F_GETFL)) == -1) return false;
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return false;

        //设置socket发信息等待
        return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1?false : true;
    }
}


