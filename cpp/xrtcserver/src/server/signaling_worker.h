#ifndef __SIGNALING_WORKER_H__
#define __SIGNALING_WORKER_H__

#include <thread>
#include <atomic>
#include <vector>
#include <memory>

#include "base/event_loop.h"
#include "base/lock_free_queue.h"
#include "rtc_base/slice.h"
#include "server/signaling_server.h"

namespace xrtc{

static void signaling_worker_recv_notify(EventLoop* el,IOWatcher* w,int fd,int events,void* data);
static void conn_io_cb(EventLoop* ,IOWatcher* ,int,int,void*);
static void conn_time_cb(EventLoop* el,TimerWatcher* w,void* data);
    
struct TcpConnection;
class SignalingWorker{
    friend void signaling_worker_recv_notify(EventLoop* el,IOWatcher* w,int fd,int events,void* data);
    friend void conn_io_cb(EventLoop* ,IOWatcher* ,int,int,void*);
    friend void conn_time_cb(EventLoop* el,TimerWatcher* w,void* data);
public:
    enum{
        QUIT = 0,
        NEW_CONN = 1
    };

    SignalingWorker(int worker_id, const SignalingServerOptions& options);
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
    void close_conn(int cfd);
    void read_query(int cfd);
    int process_query_buffer(std::shared_ptr<TcpConnection> c);
    int process_request(std::shared_ptr<TcpConnection> c,
        const rtc::Slice& header,const rtc::Slice& body);
    void close_conn(std::shared_ptr<TcpConnection> c);
    void remove_conn(std::shared_ptr<TcpConnection> c);
    void process_timeout(int cfd);

    int worker_id_;
    EventLoop* el_;
    IOWatcher* pipe_watcher_;
    int notify_recv_fd_;
    int notify_send_fd_;

    std::thread t_;
    std::atomic_bool is_start_;

    LockFreeQueue<int> q_conn_;

    std::vector<std::shared_ptr<TcpConnection>> conns_;

    SignalingServerOptions options_;
};

}





#endif