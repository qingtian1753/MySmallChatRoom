struct termios;
class SocketUtil
{
public:

    static bool inepoll(int epfd,int listenfd);
    static int setsocketnonblocknodelay(int fd);
    static void set_stdin_nonblocking();
    static void set_raw_mode(termios &orig_termios);
    static void reset_terminal(termios &orig_termios);
    static void modEpollToRead(int epfd,int fd);
    static void modEpollToWrite(int epfd,int fd);
};
