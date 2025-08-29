#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <rtc_base/sds.h>

#include "base/noncopyable.h"
#include "base/event_loop.h"

namespace xrtc{

struct TcpConnection : noncopyable{
    enum{
        STATE_HEAD = 0,
        STATE_BODY = 1
    };

    TcpConnection(int fd);
    ~TcpConnection();

    int cfd;
    char ip[64];
    int port;
    IOWatcher* io_watcher_;
    sds querybuf;
    size_t bytes_expected;
    size_t bytes_processed;

    int current_state;
};


}



#endif