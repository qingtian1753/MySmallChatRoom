#include "clientsession.h"
#include <unistd.h>

ClientSession::ClientSession(UniqueFd&& fd_)
:fd(std::move(fd_))
,write_pos(0)
,last_active_time(std::chrono::steady_clock::now())
{}

void ClientSession::write_clear()
{
    write_buffer.clear();
    write_pos=0;
}

bool ClientSession::empty()
{
    return write_buffer.empty() && out_queue.empty();
}

void ClientSession::loadOneMessage()
{
    if(write_buffer.empty() && !out_queue.empty())
    {
        write_buffer = std::move(out_queue.front());
        out_queue.pop();
    }   
}

void ClientSession::setName(const std::string& name_)
{
    name = name_;
}

void ClientSession::addReadBuffer(const char* buffer,size_t bytes_read)
{
    this->read_buffer.append(buffer,bytes_read);
}

void ClientSession::refreshActiveTime()
{
    last_active_time = std::chrono::steady_clock::now();
}
bool ClientSession::isTimeout(std::chrono::steady_clock::time_point now,int timeout_seconds) const
{
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now-last_active_time).count();
    return diff>timeout_seconds;
}

std::string& ClientSession::getReadBuffer() 
{
    return read_buffer;
}
std::string& ClientSession::getWriteBuffer()
{   
    return write_buffer;
}
size_t& ClientSession::getWritePos()
{
    return write_pos;
}

std::string& ClientSession::getName()
{
    return name;
}
void ClientSession::enqueueMsg(const std::string& msg)
{
    out_queue.emplace(msg);
}