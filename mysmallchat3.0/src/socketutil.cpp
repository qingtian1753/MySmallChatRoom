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

        //更改终端的属性
    //保证按一次收一个，不用回车才接收信息
    void setRawMode(termios& orig_termios)
    {
        struct termios raw = orig_termios;

        raw.c_lflag &= ~(ICANON | ECHO);

        raw.c_cc[VMIN]=1;
        raw.c_cc[VTIME]=0;

        tcsetattr(STDIN_FILENO,TCSANOW,&raw);
    }

    //恢复终端的属性
    void resetTerminal(termios& orig_termios)
    {
        tcsetattr(STDIN_FILENO,TCSANOW,&orig_termios);
    }

    //设置输入非阻塞
    //防止堵塞在STDIN的read这里
    void setStdinNonblocking()
    {
        int flags=fcntl(STDIN_FILENO,F_GETFL,0);

        fcntl(STDIN_FILENO,F_SETFL, flags | O_NONBLOCK);
    }

    void getTerminal(termios& orig_termios)
    {
        tcgetattr(STDIN_FILENO,&orig_termios);
    }

}

