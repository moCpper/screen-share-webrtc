#include <ev.h>      // libev/ev.h

#include "base/event_loop.h"

#define TRANS_TO_EV_MASK(mask) \
    (((mask) & EventLoop::READ ? EV_READ : 0) | \
    ((mask) & EventLoop::WRITE ? EV_WRITE : 0))

#define TRANS_FROM_EV_MASK(mask) \
    (((mask) & EV_READ ? EventLoop::READ : 0) | \
    ((mask) & EV_WRITE ? EventLoop::WRITE : 0))

namespace xrtc{

struct IOWatcher{
    IOWatcher(EventLoop* el,io_cb_t cb, void* data) :
        el(el),cb(cb),data(data){
            io.data = this;
    }
    EventLoop* el;
    ev_io io;
    io_cb_t cb;
    void* data;
};

struct TimerWatcher{
    TimerWatcher(EventLoop* el,time_cb_t cb,void* data,bool need_repeat) :
        el(el),cb(cb),data(data),need_repeat(need_repeat){
            timer.data = this;
    }
    EventLoop* el;
    struct ev_timer timer;
    time_cb_t cb;
    void* data;
    bool need_repeat;
};

// loop: 当前的事件循环实例
// io: 触发事件的IO观察器
// events: 触发的事件类型
static void generic_io_cb(struct ev_loop* loop,struct ev_io* io,int events){
    IOWatcher* watcher = static_cast<IOWatcher*>(io->data);      // io.data = this;
    watcher->cb(watcher->el,watcher,io->fd,TRANS_FROM_EV_MASK(events),watcher->data);
}

static void generic_timer_cb(struct ev_loop* loop,struct ev_timer* timer,int events){
    TimerWatcher* watcher = static_cast<TimerWatcher*>(timer->data);
    watcher->cb(watcher->el,watcher,watcher->data);
}

EventLoop::EventLoop(void* owner) :
    owner_(owner),
    loop_(ev_loop_new(EVFLAG_AUTO)){

}

EventLoop::~EventLoop(){}

void EventLoop::start(){
    ev_run(loop_);      // 启动事件循环，等待和处理事件并监控所有已注册的事件(IO、定时器等)
}

void EventLoop::stop(){
    ev_break(loop_,EVBREAK_ALL);
}

void* EventLoop::owner(){
    return owner_;
}

unsigned long EventLoop::now(){
    return static_cast<unsigned long>(ev_now(loop_) * 1000000);
}

// 负责初始化观察器并绑定回调。
// 当有事件发生时，libev会调用你指定的callback.
IOWatcher* EventLoop::create_io_event(io_cb_t cb,void* data){
    IOWatcher* w = new IOWatcher(this, cb, data);      
    ev_init(&w->io,generic_io_cb);    
    return w;
}

void EventLoop::start_io_event(IOWatcher* w,int fd,int mask){
    struct ev_io* io = &(w->io);
    if(ev_is_active(io)){
        // watcher 已经在监听中，需要更新事件
        int active_events = TRANS_FROM_EV_MASK(io->events);
        int events = active_events | mask;
        if(events == active_events){
            return;
        }

        events = TRANS_TO_EV_MASK(events);
        ev_io_stop(loop_,io);
        ev_io_set(io,fd,events);
        ev_io_start(loop_,io);
    }else{
        // watcher 尚未开始监听，首次启动
        int events = TRANS_TO_EV_MASK(mask);
        ev_io_set(io,fd,events);
        ev_io_start(loop_,io);  // 将观察器注册到事件循环
    }
}

void EventLoop::stop_io_event(IOWatcher* w,int fd,int mask){
    struct ev_io* io = &(w->io);
    int active_events = TRANS_FROM_EV_MASK(io->events);
    int events = active_events & ~mask;

    if(events == active_events){
        return;
    }

    events = TRANS_TO_EV_MASK(events);
    ev_io_stop(loop_,io);

    if(events != EV_NONE){
        ev_io_set(io,fd,events);
        ev_io_start(loop_,io);
    }
}

void EventLoop::delete_io_event(IOWatcher* w){
    struct ev_io* io = &(w->io);
    ev_io_stop(loop_,io);
    delete w;
}

TimerWatcher* EventLoop::create_timer(time_cb_t cb, void* data,bool need_repeat){
    TimerWatcher* watcher = new TimerWatcher(this, cb, data, need_repeat);
    ev_init(&watcher->timer,generic_timer_cb);
    return watcher;
}

void EventLoop::start_timer(TimerWatcher* w,unsigned int usec){
    struct ev_timer* timer = &(w->timer);
    float sec = float(usec) / 1000000.0f;

    if(!w->need_repeat){
        ev_timer_stop(loop_,timer);
        ev_timer_set(timer,sec,0);
        ev_timer_start(loop_,timer);
    }else{
        timer->repeat = sec;
        ev_timer_again(loop_,timer);
    }
}

void EventLoop::stop_timer(TimerWatcher* w){
    struct ev_timer* timer = &(w->timer);
    ev_timer_stop(loop_,timer);
}

void EventLoop::delete_timer(TimerWatcher* w){
    stop_timer(w);
    delete w;
}

}