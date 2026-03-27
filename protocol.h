#include <string>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

class protocol
{
public:

    static   std::string pack_message(const std::string& msg)
    {
        //获得数据长度
        uint32_t len = static_cast<uint32_t>(msg.length());

        //将数据长度按网络字节序排序
        uint32_t net_len = htonl(len);

        //package:用来存放打包好的数据
        std::string package;
        package.resize(4+msg.length());

        //将前四字节拷贝为长度，后四字节拷贝为数据
        memcpy(package.data(),&net_len,4);
        memcpy(package.data()+4,msg.data(),msg.length());
        return package;
    }
    
    static bool tryParseOne(std::string& read_buffer,std::string& msg)
    {

        std::string& rb =read_buffer;

        //net_len读出来是网络字节序
        uint32_t net_len = 0;

        //如果消息长度小于4，直接return
        if(rb.length() < 4)
            return false;

        memcpy(&net_len,rb.data(),4);

        //将它转为本地长度
        uint32_t body_len = ntohl(net_len);

        //如果消息没收完，直接return
        if(rb.size() < 4+body_len)  
            return false;
            
        msg = rb.substr(4,body_len);
        rb.erase(0,4+body_len);
        
        return true;
    }
};