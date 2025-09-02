#include <unistd.h>

#include "server/rtc_server.h"
#include "server/rtc_worker.h"
#include "rtc_base/logging.h"
#include "rtc_base/crc32.h"
#include "yaml-cpp/yaml.h"

namespace xrtc{

static void rtc_server_recv_notify(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int /*events*/, void* arg) {
   int msg;
   if(read(fd,&msg,sizeof(int)) != sizeof(int)){
      RTC_LOG(LS_WARNING) << "read from pipe error: " << strerror(errno);
      return;
   }

   RtcServer* server = static_cast<RtcServer*>(arg);
   server->process_notify(msg);
}

RtcServer::RtcServer() :
    el_(new EventLoop(this)),
    notify_recv_fd_(-1),
    notify_send_fd_(-1),
    pipe_watcher_(nullptr),
    is_start_(false){}

RtcServer::~RtcServer(){
    is_start_ = false;
    if(el_){
        delete el_;
    }
}

int RtcServer::init(const char* conf_file) {
    // Load configuration from the YAML file
    if(!conf_file){
        RTC_LOG(LS_WARNING) << "rtc server conf_file is null";
        return -1;
    }

    try{
        YAML::Node config = YAML::LoadFile(conf_file);
        RTC_LOG(LS_INFO) << "rtc server options: " << config;
        options_.worker_num = config["worker_num"].as<int>();
    }catch(YAML::Exception& e){
        RTC_LOG(LS_WARNING) << "catch a YAML exception,line : " << e.mark.line + 1
            << ", column: " << e.mark.column + 1 << ", error : " << e.msg;
        return -1;
    }
    
    int fds[2];
    if(pipe(fds)){
        RTC_LOG(LS_WARNING) << "create pipe error :" << strerror(errno);
        return -1;
    }

    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];

    pipe_watcher_ = el_->create_io_event(rtc_server_recv_notify,this);
    el_->start_io_event(pipe_watcher_,notify_recv_fd_,EventLoop::READ);

    for(int i = 0; i < options_.worker_num; ++i){
        if(create_worker(i) != 0){
            return -1;
        }
    }


    return 0;
}

int RtcServer::create_worker(int worker_id){
    RTC_LOG(LS_INFO) << "create rtc worker,worker_id : " << worker_id  ;

    std::shared_ptr<RtcWorker> worker{std::make_shared<RtcWorker>(worker_id, options_)};
    if(!worker){
        RTC_LOG(LS_WARNING) << "create rtc worker failed,worker_id : " << worker_id;
        return -1;
    }

    if(worker->init() < 0){
        return -1;
    }

    if(worker->start() < 0){
        return -1;
    }

    workers_.push_back(worker);

    return 0;
}

std::shared_ptr<RtcWorker> RtcServer::get_worker(const std::string& stream_name) {
    if(workers_.size() == 0 || workers_.size() != (size_t)options_.worker_num){
        return nullptr;
    }
    uint32_t num = rtc::ComputeCrc32(stream_name);
    size_t index = num % options_.worker_num;

    return workers_[index];
}

void RtcServer::process_rtc_msg(){
    auto msg = pop_msg();
    if(!msg){
        return;
    }
    std::shared_ptr<RtcWorker> worker{get_worker(msg->stream_name)};
    if(worker){
        worker->send_rtc_msg(msg);
    }
   
}

void RtcServer::process_notify(int msg){
    switch(msg){
        case QUIT:
            stop_();
            break;
        case RTC_MSG:
            process_rtc_msg();
            break;
        default:
            RTC_LOG(LS_WARNING) << "Unknown message: " << msg;
            break;
    }
}

void RtcServer::stop_(){
    if(!is_start_){
        RTC_LOG(LS_WARNING) << "RtcServer is not started";
        return;
    }

    el_->delete_io_event(pipe_watcher_);
    el_->stop();
    close(notify_recv_fd_);
    close(notify_send_fd_);

    for(auto worker : workers_){
        if(worker){
            worker->stop();
            worker->join();
        }
    }
}

bool RtcServer::start(){
    if(is_start_){
        RTC_LOG(LS_WARNING) << "rtcserver is already started";
        return false;
    }
    is_start_ = true;

    t_ = std::thread([=](){
        RTC_LOG(LS_INFO) << "rtcserver thread run";
        el_->start();
        RTC_LOG(LS_INFO) << "rtcserver thread stop";
        is_start_ = false;
    });

    return true;
}

void RtcServer::stop(){
    notify(QUIT);
}

int RtcServer::notify(int msg){
    int written = write(notify_send_fd_,&msg,sizeof(int));
    return written == sizeof(int) ? 0 : -1;
}   

void RtcServer::join(){
    if(t_.joinable()){
        t_.join();
    }
}

int RtcServer::send_rtc_msg(std::shared_ptr<RtcMsg> msg){
    if(!msg){
        return -1;
    }

    push_msg(msg);

    return notify(RTC_MSG);
}

void RtcServer::push_msg(std::shared_ptr<RtcMsg> msg){
    std::unique_lock<std::mutex> lock(q_msg_mtx_);
    msg_queue_.push(msg);
}

std::shared_ptr<RtcMsg> RtcServer::pop_msg(){
    std::unique_lock<std::mutex> lock(q_msg_mtx_);
    if(msg_queue_.empty()){
        return nullptr;
    }
    std::shared_ptr<RtcMsg> msg = msg_queue_.front();
    msg_queue_.pop();
    return msg;
}

}