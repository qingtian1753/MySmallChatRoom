#include "clientstate.h"

ClientState::ClientState()
:fd(-1),write_pos(0)
,ping_msg("/ping")
,last_active_time(std::chrono::steady_clock::now())
,waiting_pong(false)
{}

    //清除发送缓冲区，下标清0
void ClientState::write_clear()
{
    write_buffer.clear();
    write_pos=0;
}



bool ClientState::empty()
{
    return write_buffer.empty() && out_queue.empty();
}

void ClientState::loadOneMessage()
{
    if(write_buffer.empty() && !out_queue.empty())
    {
        write_buffer = std::move(out_queue.front());
        out_queue.pop();
    }   
}

void ClientState::inputClear()
{
    input_buffer.clear();
}

void ClientState::refreshActiveTime()
{
    last_active_time = std::chrono::steady_clock::now();
    waiting_pong = false;
}

bool ClientState::isTimeout(std::chrono::steady_clock::time_point now,int timeout_seconds) const
{
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now-last_active_time).count();
    return diff>timeout_seconds;
}

void ClientState::enqueueMsg(const std::string& msg)
{
    out_queue.emplace(msg);
}


void ClientState::addReadBuffer(const char* buffer,size_t bytes_read)
{
    read_buffer.append(buffer,bytes_read);
}
std::string& ClientState::getReadBuffer()
{
    return read_buffer;
}
std::string& ClientState::getWriteBuffer()
{
    return write_buffer;
}
size_t& ClientState::getWritePos()
{   
    return write_pos;
}

std::string& ClientState::getInputBuffer()
{
    return input_buffer;
}

UniqueFd& ClientState::getFd()
{
    return fd;
}

bool& ClientState::getWaitingPong()
{
    return waiting_pong;
}
std::string& ClientState::getPingMsg()
{
    return ping_msg;
}