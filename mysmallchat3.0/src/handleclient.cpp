#include "handleclient.h"
#include "clientsession.h"
#include "epoll.h"
#include "protocol.h"
#include <unordered_map>

namespace handle{


    std::string extractCommand(const std::string& msg)
    {
        std::size_t pos = msg.find(' ');
        if (pos == std::string::npos)
        {
            return msg;
        }
        return msg.substr(0, pos);
    }
    std::unordered_map<std::string,CmdHandler>& getTable()
    {
        static std::unordered_map<std::string,CmdHandler>table={
            {"/name",handleName},
            {"/list",handleList},
            {"/id",handleId},
            {"/quit",handleQuit},
            {"/msg",handleMsg},
            {"/help",handleHelp},
            {"/ping",handlePing},
        };
        return table;
    }
    void handleCommand(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
       auto& table = getTable();
       std::string cmd = extractCommand(msg);
       auto it = table.find(cmd);

       //如果没找到这个指令或者执行失败了，则发送非法指令
       if(it == table.end() || !it->second(fd,msg,clients,epoll))
            handleIllegalCmd(fd,msg,clients,epoll);

    }


    bool handleName(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        //处理创建名称
        if(msg.find("/name ")==0)
        {
            //判断名字是否合法
            bool illegal = true;

            //第一个不是空格的元素的下标
            int pos1;
            //判断name后面是不是全是空格
            for(int i=6;i<msg.length();++i)
            {
                if(msg[i]!=' ' && illegal)
                {
                    pos1 = i;
                    illegal = false;
                    break;
                }
            }

            //如果非法
            if(illegal)
            {
                std::string message = "the name is illegal!";
                buildSystemMessage(fd,message,clients,epoll);
                return true;
            }

            std::string new_name = msg.substr(pos1,msg.length()-pos1);

            //判断名字是否存在
            for(auto& it : clients)
            {
                if(it.second.getName() == new_name)
                {
                    std::string message = "the name is already existing!";
                    buildSystemMessage(fd,message,clients,epoll);
                    return true;
                }
            }
            auto it =clients.find(fd);
            if(it == clients.end())
                return false;
            it->second.setName(new_name);
            std::string message = new_name + " registered!";
            broadcastSystemMessage(message,clients,epoll);
            return true;
        }
        return false;
    }
    bool handleList(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg == "/list")
        {
            std::string message;
            for(auto& it : clients)
            {
                message+=it.second.getName() + "\n";
            }
            //"\n"全部由广播或者单发函数来加，这里最后不能加"\n"，所以要pop
            if(!message.empty())
                message.pop_back();

            buildSystemMessage(fd,message,clients,epoll);
            return true;
        }
        return false;
    }

    bool handleQuit(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg == "/quit")
        {
            auto it =clients.find(fd);
            if(it == clients.end())
                return false;
            std::string message = it->second.getName() + " quit!";
            
            closeClient(fd,epoll,clients);
            broadcastSystemMessage(message,clients,epoll);
            return true;
        }

        return false;
    }

    bool handleId(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg == "/id")
        {
            auto it = clients.find(fd);
            if(it == clients.end())
                return false; 
            std::string message = it->second.getName() ;
            buildSystemMessage(fd,message,clients,epoll);
            return true;
        }
        return false;
    }

    bool handleMsg(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg.find("/msg ") == 0)
        {
            int pos1 = msg.find(" ",5);
            if(pos1 == std::string::npos)
                return false;
            std::string target_name = msg.substr(5,pos1-5);
            for(auto& it : clients)
            {
                //如果找到了目标，则进行私聊
                if(it.second.getName() == target_name)
                {
                    auto fdit = clients.find(fd);
                    if(fdit == clients.end())
                        return false;
                    std::string sentence = msg.substr(pos1+1,msg.length()-pos1-1);
                    std::string message = "[private]<"+fdit->second.getName()+">"+sentence ;
                    queueMessage(it.first,message,clients,epoll);

                    message = "[private]<you to "+it.second.getName()+">"+sentence;
                    queueMessage(fd,message,clients,epoll);
                    return true;
                }
            }
            std::string message = "the client is not existing!";
            buildSystemMessage(fd,message,clients,epoll);
            return true;
        }
        return false;
    }

    void handleIllegalCmd(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        std::string message = "illegal message!";
        buildSystemMessage(fd,message,clients,epoll);
    }

    bool handleHelp(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg=="/help")
        {
            auto& table = getTable();
            std::string message;
            for(auto& it: table)
            {
                message+=it.first+"\n";
            }
            if(!message.empty())
                message.pop_back();       
            queueMessage(fd,message,clients,epoll);
            return true;
        }
        return false;
    }
    bool handlePing(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        if(msg != "/ping")
            return false;

        auto it = clients.find(fd);
        if(it == clients.end())
            return false;

        //服务端收到/ping后发/pong让客户端刷新最后响应时间
        queueMessage(fd,"/pong",clients,epoll);
        it->second.refreshActiveTime();
        return true;
    }
    void handleBroadcast(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        auto fdit = clients.find(fd);
        if(fdit == clients.end())
            return ;
        std::string message = fdit->second.getName()+": "+msg;
        for(auto& it : clients)
        {
            queueMessage(it.first,message,clients,epoll);
        }
    }

    void broadcastSystemMessage(const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        std::string message = "[system]" + msg ;
        for(auto& it : clients)
        {
            queueMessage(it.first,message,clients,epoll);
        }
    }

    void queueMessage(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        auto it = clients.find(fd);
        if(it == clients.end())
            return;

        it->second.enqueueMsg(protocol::pack_message(msg+"\n"));
        epoll.modEpollToWrite(fd);
    }

    void buildSystemMessage(int fd,const std::string& msg,std::unordered_map<int,ClientSession>&clients,Epoll& epoll)
    {
        std::string message = "[system]" + msg;
        queueMessage(fd,message,clients,epoll);
    }

    void closeClient(int fd,Epoll& epoll,std::unordered_map<int,ClientSession>&clients)
    {
        epoll.delFd(fd);
        //unorderedmap在erase时会调用ClientSession的析构函数
        clients.erase(fd);
    }

}

