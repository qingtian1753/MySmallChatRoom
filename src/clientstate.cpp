#include "clientstate.h"

ClientState::ClientState()
:fd(-1),write_pos(0){}

    //清除发送缓冲区，下标清0
void ClientState::write_clear()
     {
         write_buffer.clear();
         write_pos=0;
     }