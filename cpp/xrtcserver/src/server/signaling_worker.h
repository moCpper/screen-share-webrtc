#ifndef __SIGNALING_WORKER_H__
#define __SIGNALING_WORKER_H__

#include <thread>
#include <atomic>
#include <vector>
#include <memory>

#include "base/event_loop.h"
#include "base/lock_free_queue.h"

namespace xrtc{

static void signaling_worker_recv_notify(EventLoop* el,IOWatcher* w,int fd,int events,void* data);

static void conn_io_cb(EventLoop* ,IOWatcher* ,int,int,void*);
    
struct TcpConnection;
class SignalingWorker{
    friend void signaling_worker_recv_notify(EventLoop* el,IOWatcher* w,int fd,int events,void* data);
    friend void conn_io_cb(EventLoop* ,IOWatcher* ,int,int,void*);
public:
    enum{
        QUIT = 0,
        NEW_CONN = 1
    };

    SignalingWorker(int worker_id);
    ~SignalingWorker();

    int init();
    bool start();
    void stop();
    int notify(int msg);
    void join();
    int notify_new_conn(int cfd);
    
private:
    void process_notify(int msg);
    void stop_();
    void new_conn(int cfd);
    void read_query(int cfd);

    int worker_id_;
    EventLoop* el_;
    IOWatcher* pipe_watcher_;
    int notify_recv_fd_;
    int notify_send_fd_;

    std::thread t_;
    std::atomic_bool is_start_;

    LockFreeQueue<int> q_conn_;

    std::vector<std::shared_ptr<TcpConnection>> conns_;
};

}





#endif