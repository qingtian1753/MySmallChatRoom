#include "src/chatserver.h"


int main(int arvc,char* arvg[])
{
    if(arvc != 2 )
    {
        LOG_ERROR("Usage: <port>!");
        return 1;
    }
    ChatServer server(1024,std::atoi(arvg[1]));
    if(!server.init())
    {
        LOG_ERROR("server initialize failed!");
        return 1;
    }
    server.run();
}