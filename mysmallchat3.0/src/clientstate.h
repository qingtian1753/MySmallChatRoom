#include <string>
#include <queue>
#include <chrono>
#include "uniquefd.h"

class ClientState
{
private:
    std::queue<std::string> out_queue;
    std::string write_buffer;
    std::string read_buffer;
    std::string input_buffer;
    size_t write_pos;
    UniqueFd fd;

    bool waiting_pong;
    std::string ping_msg;
    std::chrono::steady_clock::time_point last_active_time;

public:

    ClientState();
    
    //清除发送缓冲区，下标清0
    void write_clear();


    //判断缓冲区和缓冲队列是否为空
    bool empty();

    //装填消息
    void loadOneMessage();

    void inputClear();

    void enqueueMsg(const std::string& msg);

    //如果收到了服务端消息则刷新时间,并且将waiting_pong置为false
    //因为只要收到了消息就证明连接没问题
    void refreshActiveTime();
    //如果超时为发送就发送/ping
    bool isTimeout(std::chrono::steady_clock::time_point now,int timeout_seconds) const;

    void addReadBuffer(const char* buffer,size_t bytes_read);
    std::string& getReadBuffer();
    std::string& getWriteBuffer();
    size_t& getWritePos();
    std::string& getInputBuffer();
    UniqueFd& getFd();
    bool& getWaitingPong();
    std::string& getPingMsg();

};