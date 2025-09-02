#include <unistd.h>

#include "server/rtc_worker.h"
#include "rtc_base/logging.h"
#include "server/signaling_worker.h"

namespace xrtc{

static void rtc_worker_recv_notify(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int /*events*/, void* arg) {
   int msg;
   if(read(fd,&msg,sizeof(int)) != sizeof(int)){
      RTC_LOG(LS_WARNING) << "read from pipe error: " << strerror(errno);
      return;
   }

   RtcWorker* worker = static_cast<RtcWorker*>(arg);
   worker->process_notify(msg);
}

RtcWorker::RtcWorker(int id, const RtcServerOptions& options): 
  worker_id_(id),
  options_(options), 
  el_(new EventLoop(this)),
  notify_recv_fd_(-1),
  notify_send_fd_(-1),
  pipe_watcher_(nullptr),
  is_start_(false){
}

RtcWorker::~RtcWorker(){
    if(el_){ 
        delete el_;
        el_ = nullptr;
    }
    is_start_ = false;
}

int RtcWorker::init(){
    int fds[2];
    if(pipe(fds)){
        RTC_LOG(LS_WARNING) << "create pipe error :" << strerror(errno);
        return -1;
    }

    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];

    pipe_watcher_ = el_->create_io_event(rtc_worker_recv_notify,this);
    el_->start_io_event(pipe_watcher_,notify_recv_fd_,EventLoop::READ);

    return 0;
}

bool RtcWorker::start(){
    if(is_start_){
        RTC_LOG(LS_WARNING) << "rtc worker already start , worker_id : " << worker_id_;
    }
    is_start_ = true;

    t_ = std::thread([=](){
        RTC_LOG(LS_INFO) << "rtc worker event loop start , worker_id : " << worker_id_;
        el_->start();
        RTC_LOG(LS_INFO) << "rtc worker event loop stop , worker_id : " << worker_id_;
        is_start_ = false;
    });
    return true;
}

void RtcWorker::process_push(std::shared_ptr<RtcMsg> msg){
    std::string offer{"offer"};

    msg->sdp = offer;

    SignalingWorker* worker = static_cast<SignalingWorker*>(msg->worker);
    if(worker){
        worker->send_rtc_msg(msg);
    }
}

void RtcWorker::process_rtc_msg(){
    std::shared_ptr<RtcMsg> msg;
    if(!pop_msg(&msg)){
        return;
    }

    RTC_LOG(LS_INFO) << " cmdno : " << msg->cmdno << " uid : " << msg->uid
        << " stream_name : " << msg->stream_name
        << " audio : " << msg->audio
        << " video : " << msg->video
        << " log_id : " << msg->log_id
        << "  rtc worker receive msg, worker_id: " << worker_id_;

    switch(msg->cmdno){
        case CMDNO_PUSH:
            process_push(msg);
            break;
        default:
            RTC_LOG(LS_WARNING) << "unknown cmdno : " << msg->cmdno;
    }

}

void RtcWorker::process_notify(int msg){
    switch(msg){
        case QUIT:
            stop_();
            break;
        case RTC_MSG:
            process_rtc_msg();
            break;
        default:
            RTC_LOG(LS_WARNING) << "RtcWorker unknown msg : " << msg;
            break;
    }
}

int RtcWorker::notify(int msg){
    int written = write(notify_send_fd_, &msg, sizeof(msg));
    return written == sizeof(int) ? 0 : -1;
}

void RtcWorker::stop(){
    notify(QUIT);
}

void RtcWorker::join(){
    if(t_.joinable()){
        t_.join();
    }
}

void RtcWorker::stop_(){
    if(!is_start_){
        RTC_LOG(LS_WARNING) << "rtc worker not running, worker_id : " << worker_id_;
        return;
    }

    el_->delete_io_event(pipe_watcher_);
    el_->stop();
    close(notify_recv_fd_);
    close(notify_send_fd_);
}

void RtcWorker::push_msg(std::shared_ptr<RtcMsg> msg){
    msg_queue_.produce(msg);
}

bool RtcWorker::pop_msg(std::shared_ptr<RtcMsg>* msg){
    return msg_queue_.consume(msg);
}

int RtcWorker::send_rtc_msg(std::shared_ptr<RtcMsg> msg){
     // 将消息投递至worker队列
     push_msg(msg);
     return notify(RTC_MSG);
}

}