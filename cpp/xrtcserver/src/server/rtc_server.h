#ifndef __RTC_SERVER_H_
#define __RTC_SERVER_H_

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <queue>
#include <vector>

#include "base/event_loop.h"
#include "xrtcserver_def.h"

namespace xrtc{

struct RtcServerOptions{
    int worker_num;
};

static void rtc_server_recv_notify(EventLoop*,IOWatcher*,int,int,void*);

class RtcWorker;
class RtcServer{
    friend void rtc_server_recv_notify(EventLoop*,IOWatcher*,int,int,void*);
public:
    enum{
        QUIT = 0,
        RTC_MSG = 1
    };
    RtcServer();
    ~RtcServer();

    int init(const char* conf_file);
    bool start();
    void stop();
    void join();
    int notify(int msg);
    int send_rtc_msg(std::shared_ptr<RtcMsg>);
private:
    void push_msg(std::shared_ptr<RtcMsg>);
    std::shared_ptr<RtcMsg> pop_msg();
    int create_worker(int worker_id);
    void process_notify(int msg);
    void stop_();
    void process_rtc_msg();
    std::shared_ptr<RtcWorker> get_worker(const std::string& stream_name);

    EventLoop* el_;
    RtcServerOptions options_;

    IOWatcher* pipe_watcher_;
    int notify_recv_fd_;
    int notify_send_fd_;

    std::thread t_;
    std::atomic_bool is_start_;

    std::queue<std::shared_ptr<RtcMsg>> msg_queue_;
    mutable std::mutex q_msg_mtx_;

    std::vector<std::shared_ptr<RtcWorker>> workers_;
};

}

#endif