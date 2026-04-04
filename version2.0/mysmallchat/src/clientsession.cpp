#include "clientsession.h"
#include <unistd.h>

ClientSession::ClientSession(UniqueFd&& fd_)
:fd(std::move(fd_))
,write_pos(0)
{}

void ClientSession::write_clear()
{
    write_buffer.clear();
    write_pos=0;
}
