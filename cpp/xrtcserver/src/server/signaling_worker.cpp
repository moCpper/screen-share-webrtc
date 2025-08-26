#include <unistd.h>
#include <iostream>

#include "server/signaling_worker.h"
#include "rtc_base/logging.h"

namespace xrtc{

static void signaling_worker_recv_notify(EventLoop* el,IOWatcher* w,int fd,
    int events,void* data){
    int msg;
    if(read(fd,&msg,sizeof(int)) != sizeof(int)){
        RTC_LOG(LS_WARNING) << "SignalingWorker read error : " << strerror(errno)
            << ", errno : " << errno;
        return;
    }

    SignalingWorker* worker = static_cast<SignalingWorker*>(data);
    worker->process_notify(msg);
}

SignalingWorker::SignalingWorker(int worker_id) 
    : worker_id_(worker_id),
    is_start_(false),
    el_(new EventLoop(this)),
    notify_recv_fd_(-1),
    notify_send_fd_(-1){}

SignalingWorker::~SignalingWorker(){}

int SignalingWorker::init(){
    int fds[2];
    if(pipe(fds)){
        RTC_LOG(LS_WARNING) << "create pipe error : " << strerror(errno)
            << ", errno : " << errno;
        return -1;
    }

    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];

    pipe_watcher_ = el_->create_io_event(signaling_worker_recv_notify,this);
    el_->start_io_event(pipe_watcher_,notify_recv_fd_,EventLoop::READ);

    return 0;
}

bool SignalingWorker::start(){
    if(is_start_){
        RTC_LOG(LS_WARNING) << "SignalingWorker is already started";
        return false;
    }
    is_start_ = true;

    t_ = std::thread([=](){
        RTC_LOG(LS_INFO) << "SignalingWorker thread run";
        el_->start();
        RTC_LOG(LS_INFO) << "SignalingWorker thread stop";
        is_start_ = false;
        std::cout << "@222222222222222222222222222222" << std::endl;
    });

    return true;
}

void SignalingWorker::stop(){
    if(notify(QUIT) < 0){
        RTC_LOG(LS_WARNING) << "SignalingWorker stop error";
    }
}

int SignalingWorker::notify(int msg){
    int written = write(notify_send_fd_, &msg, sizeof(msg));
    return written == sizeof(int) ? 0 : -1;
}

void SignalingWorker::process_notify(int msg){
    switch(msg){
        case QUIT:
            stop_();
            break;
        case NEW_CONN:
            int fd;
            if(q_conn_.consume(&fd)){
                new_conn(fd);
            }
            break;
        default:
            RTC_LOG(LS_WARNING) << "SignalingWorker unknown msg : " << msg;
            break;
    }
}

void SignalingWorker::new_conn(int cfd){
    // 处理新连接
    RTC_LOG(LS_INFO) << "SignalingWorker " << worker_id_ 
        << " handle new connection, cfd : " << cfd;
    
}

void SignalingWorker::stop_(){
    if(!is_start_){
        RTC_LOG(LS_WARNING) << "SignalingWorker is not started";
        return;
    }

    el_->delete_io_event(pipe_watcher_);
    el_->stop();

    close(notify_recv_fd_);
    close(notify_send_fd_);
}

void SignalingWorker::join(){
    if(t_.joinable()){
        t_.join();
    }
}

int SignalingWorker::notify_new_conn(int cfd){
    q_conn_.produce(cfd);
    return notify(NEW_CONN);
}

}