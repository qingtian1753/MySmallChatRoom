struct termios;
namespace SocketUtil
{
    // 获取终端属性
    void getTerminal(termios& orig_termios);

    bool setsocketnonblocknodelay(int fd);

    void setRawMode(termios& orig_termios);

    //恢复终端属性
    void resetTerminal(termios& orig_termios);

    void setStdinNonblocking();
};
