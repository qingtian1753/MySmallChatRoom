#include <string>
#include <queue>
#include <chrono>
#include "uniquefd.h"

class ClientSession
{
private:
    std::string read_buffer;
    std::string write_buffer;
    UniqueFd fd;
    size_t write_pos;
    std::string name;

    std::queue<std::string>out_queue;

    //记录客户最后一次活跃时间，为什么使用steady_clock
    //因为他是个单调时钟，不会被系统时间影响，
    //适合求时间差
    std::chrono::steady_clock::time_point last_active_time;
    
public:

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

    //判断缓冲区和缓冲队列是否为空
    bool empty();

    //装填消息
    void loadOneMessage();

    void setName(const std::string& name);
    void addReadBuffer(const char* buffer,size_t bytes_read);
    std::string& getReadBuffer();
    std::string& getWriteBuffer();
    size_t& getWritePos();
    void refreshActiveTime();
    bool isTimeout(std::chrono::steady_clock::time_point now,int timeout_seconds) const;
    std::string& getName();
    void enqueueMsg(const std::string& msg);
};