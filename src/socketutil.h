struct termios;
namespace SocketUtil
{
    bool setsocketnonblocknodelay(int fd);
    void set_stdin_nonblocking();
    void set_raw_mode(termios &orig_termios);
    void reset_terminal(termios &orig_termios);
};
