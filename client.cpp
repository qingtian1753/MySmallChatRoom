#include "chatclient.h"
#include <iostream>

int main(int argc ,char ** argv)
{
    ChatClient cc;
    if(argc!=3)
    {
        std::cerr<<"Usage "<<"<server_ip><port>\n";
        return 1;
    }
    if(!cc.initclient(argv[1],std::stoi(argv[2])))
    {
        std::cerr<<"client initialize failed!"<<std::endl;
        return 1;
    }
    cc.run();
    return 0;
}