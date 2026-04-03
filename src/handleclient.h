#pragma once
#include <string>
#include <unordered_map>
class Epoll;
struct ClientSession;

namespace handle{

    void handleCommand(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);
    
    bool handleName(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    bool handleList(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    bool handleQuit(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    bool handleId(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    bool handleMsg(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    void handleIllegalCmd(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    //广播消息给所有人
    void handleBroadcast(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    //将系统消息广播给所有人
    void broadcastSystemMessage(const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    //发送消息给个人
    void buildSystemMessage(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);

    void queueMessage(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll);
}