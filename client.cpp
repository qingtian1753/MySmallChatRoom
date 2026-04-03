#include "src/chatclient.h"
#include  "src/Log.h"
int main(int argc ,char ** argv)
{
    ChatClient cc(10);
    if(argc!=3)
    {
        LOG_ERROR("Usage <server_ip><port>");
        return 1;
    }
    if(!cc.initclient(argv[1],std::stoi(argv[2])))
    {
        LOG_ERROR("client initialize failed!");
        return 1;
    }
    cc.run();
    return 0;
}