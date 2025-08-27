#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include "base/event_loop.h"

namespace xrtc{

struct TcpConnection{
    TcpConnection(int fd);
    ~TcpConnection();

    int cfd;
    char ip[64];
    int port;
    IOWatcher* io_watcher_;
};


}



#endif