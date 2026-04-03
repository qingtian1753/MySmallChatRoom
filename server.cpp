#include "src/chatserver.h"
#include <iostream>

#define SERVER_PORT 7711

int main()
{
    ChatServer server(1024,SERVER_PORT);
    if(!server.init())
    {
        LOG_ERROR("server initialize failed!");
        return 1;
    }
    server.run();
}