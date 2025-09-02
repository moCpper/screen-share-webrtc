#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <list>

#include "rtc_base/sds.h"
#include "rtc_base/slice.h"
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
    TimerWatcher* timer_watcher_;
    sds querybuf;
    size_t bytes_expected;
    size_t bytes_processed;

    int current_state;
    unsigned long last_interaction = 0;

    std::list<rtc::Slice> reply_list;
    size_t cur_resp_pos;
};


}



#endif