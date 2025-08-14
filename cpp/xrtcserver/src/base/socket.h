#ifndef __BASE_SOCKET_H__
#define __BASE_SOCKET_H__

#include <string>
#include <sys/socket.h>

namespace xrtc{

int create_tcp_server(const char* addr, int port);

}

#endif