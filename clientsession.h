#include <string>
#include <deque>
struct ClientSession
{

    size_t write_pos;
    std::string name;
    std::string read_buffer;
    std::string write_buffer;
    std::deque<std::string>out_queue;
    
    ClientSession();
    void write_clear();
    ~ClientSession();

};