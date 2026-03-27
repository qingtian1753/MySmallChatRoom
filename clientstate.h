#include <string>
#include <deque>

struct ClientState
{
    std::deque<std::string> out_queue;
    std::string write_buffer;
    std::string read_buffer;
    std::string input_buffer;
    size_t write_pos;
    int fd;
    ClientState();

    //清除发送缓冲区，下标清0
    void write_clear();
};