#ifndef _RTC_WORKER_H_
#define _RTC_WORKER_H_

#include <thread>
#include <mutex>
#include <atomic>

#include "base/event_loop.h"
#include "server/rtc_server.h"
#include "base/lock_free_queue.h"
#include "xrtcserver_def.h"

namespace xrtc{

static void rtc_worker_recv_notify(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int /*events*/, void* arg);
 
class RtcWorker{
    friend void rtc_worker_recv_notify(EventLoop*,IOWatcher*,int,int,void*);
public:
    enum{
        QUIT = 0,
        RTC_MSG = 1
    };
    RtcWorker(int id, const RtcServerOptions& options);
    ~RtcWorker();

    int init();
    bool start();
    void stop();
    void join();
    int notify(int msg);
    void push_msg(std::shared_ptr<RtcMsg>);
    bool pop_msg(std::shared_ptr<RtcMsg>*);
    int send_rtc_msg(std::shared_ptr<RtcMsg>);
private:
    void process_notify(int msg);

    void stop_();
    void process_rtc_msg();
    void process_push(std::shared_ptr<RtcMsg> msg);

    int worker_id_;
    RtcServerOptions options_;
    EventLoop* el_;

    IOWatcher* pipe_watcher_;
    int notify_send_fd_;
    int notify_recv_fd_;

    std::thread t_;
    std::atomic_bool is_start_;

    LockFreeQueue<std::shared_ptr<RtcMsg>> msg_queue_;
};

}

#endif