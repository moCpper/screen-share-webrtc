#ifndef __SIGNALING_SERVER_H__
#define __SIGNALING_SERVER_H__

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>

#include "base/event_loop.h"

namespace xrtc{

static void signaling_server_recv_notify(EventLoop* el,IOWatcher* w,int fd, int events,void* data);
static void accept_new_conn(EventLoop* el,IOWatcher* w,int fd, int events,void* data);

struct SignalingServerOptions{
   std::string host;
   int port;
   int worker_num;
   int connection_timeout;
};

class SignalingWorker;
class SignalingServer{
   friend void signaling_server_recv_notify(EventLoop* el,IOWatcher* w,int fd, int events,void* data);
   friend void accept_new_conn(EventLoop* el,IOWatcher* w,int fd, int events,void* data);
public:
   enum{
      QUIT = 0
   };

   SignalingServer();
   ~SignalingServer();

   int init(const char* conf_file);
   bool start();
   void stop();
   int notify(int msg);
   void join();

private:
   void process_notify(int msg);
   
   void stop_();
   int create_worker(int worker_id);
   void dispatch_new_conn(int cfd);

   SignalingServerOptions options_;
   EventLoop* el_;

   IOWatcher* io_watcher_;
   IOWatcher* pipe_watcher_;
   
   int notify_recv_fd_;
   int notify_send_fd_;

   int listen_fd_;
   std::vector<SignalingWorker*> workers_;
   int next_worker_index_;

   std::thread t_;
   std::atomic_bool is_start_;
};

}

#endif