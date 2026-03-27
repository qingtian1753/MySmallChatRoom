#include "chatserver.h"
#include <iostream>

#define SERVER_PORT 7711

int main()
{
    ChatServer server(SERVER_PORT);
    if(!server.init())
    {
        std::cerr<<"server initialize failed!"<<std::endl;
        return 1;
    }
    server.run();
}