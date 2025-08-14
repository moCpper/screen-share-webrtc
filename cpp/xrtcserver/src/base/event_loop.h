#ifndef __BASE_EVENTLOOP_H__
#define __BASE_EVENTLOOP_H__

struct ev_loop;

namespace xrtc{

class EventLoop;
class TimerWatcher;
struct IOWatcher;

using io_cb_t = void (*)(EventLoop* el,IOWatcher* w,int fd,int events,void* data);
using time_cb_t = void(*)(EventLoop* el,TimerWatcher* w,void* data);

class EventLoop{
public:
    enum{
        READ = 0X1,
        WRITE = 0X2
    };

    EventLoop(void* owner);
    ~EventLoop();

    void start();
    void stop();

    IOWatcher* create_io_event(io_cb_t cb,void* data);
    void start_io_event(IOWatcher* w,int fd,int mask);
    void stop_io_event(IOWatcher* w,int fd,int mask);
    void delete_io_event(IOWatcher* w);

    TimerWatcher* create_timer(time_cb_t cb, void* data,bool need_repeat);
    void start_timer(TimerWatcher* w,unsigned int usec);
    void stop_timer(TimerWatcher* w);
    void delete_timer(TimerWatcher* w);
private:
    void* owner_;
    struct ev_loop* loop_;
};

}

#endif