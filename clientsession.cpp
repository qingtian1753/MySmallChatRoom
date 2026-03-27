#include "clientsession.h"


ClientSession::ClientSession()
:write_pos(0)
,name(""),read_buffer("")
,write_buffer(""){}

ClientSession::~ClientSession(){}

void ClientSession::write_clear()
{
    write_buffer.clear();
    write_pos=0;
}
