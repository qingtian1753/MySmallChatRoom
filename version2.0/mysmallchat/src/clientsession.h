#include <string>
#include <queue>
#include "uniquefd.h"

struct ClientSession
{
    UniqueFd fd;
    size_t write_pos;
    std::string name;
    std::string read_buffer;
    std::string write_buffer;
    std::queue<std::string>out_queue;
    
    //没有默认构造函数，unordered_map<>不能使用[]运算符来访问
    //[]默认这个类必须有默认构造，如果没有编译器会报错
    ClientSession(UniqueFd&& fd_);

    //删掉拷贝构造
    ClientSession& operator=(const ClientSession&) = delete;
    ClientSession(const ClientSession&) =delete;

    //允许移动构造，必须显示声明否则报错
    ClientSession& operator=(ClientSession&&) = default;
    ClientSession(ClientSession&&) = default;
    void write_clear();
};