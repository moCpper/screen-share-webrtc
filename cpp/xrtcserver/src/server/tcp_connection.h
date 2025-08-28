#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <rtc_base/sds.h>

#include "base/event_loop.h"

namespace xrtc{

struct TcpConnection{
    TcpConnection(int fd);
    ~TcpConnection();

    int cfd;
    char ip[64];
    int port;
    IOWatcher* io_watcher_;
    sds querybuf;
    size_t bytes_expected;
    size_t bytes_processed;
};


}



#endif