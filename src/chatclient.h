#include "clientstate.h"
#include "socketutil.h"
#include <termios.h>

class Epoll;
class ChatClient
{
private:
    Epoll epoll;
    
    termios orig_termios;
    ClientState states;
private:
    
    bool try_send(int fd);
    bool queue_message_and_try_send( std::string& msg);
    void clear_cur_line();
    void redraw_input_line();
    void handle_one_message(std::string& msg);
    void handle_read_buffer();
    bool recv_all(const int fd);
    bool handle_stdin(int client_fd);

public:
    bool initclient(const char* ip,int port);
    void run();
    ChatClient(int MAX_EVENTS = 1024);
    ~ChatClient();
};