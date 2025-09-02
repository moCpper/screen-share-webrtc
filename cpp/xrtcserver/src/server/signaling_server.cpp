#include <rtc_base/logging.h>
#include <yaml-cpp/yaml.h>
#include <unistd.h>

#include "server/signaling_server.h"
#include "server/signaling_worker.h"
#include "base/socket.h"

namespace xrtc{

// pipe read callback
static void signaling_server_recv_notify(EventLoop* el,IOWatcher* w,int fd, int events,void* data){
    int msg;
    if(read(fd,&msg,sizeof(int)) != sizeof(int)){
        RTC_LOG(LS_WARNING) << "signaling server recv notify read failed";
        return;
    }

    SignalingServer* server = static_cast<SignalingServer*>(data);
    server->process_notify(msg);
}

static void accept_new_conn(EventLoop* el,IOWatcher* w,int fd, int events,void* data){
    int cfd;
    char cip[128];
    int cport;

    cfd = tcp_accept(fd,cip,&cport);
    if(cfd == -1){
        RTC_LOG(LS_ERROR) << "tcp_accept error ";
        return;
    }

    RTC_LOG(LS_INFO) << "new connection accepted, fd: " << cfd
                     << ", ip: " << cip << ", port: " << cport;

    SignalingServer* server = static_cast<SignalingServer*>(data);
    server->dispatch_new_conn(cfd);
}

SignalingServer::SignalingServer() : 
    el_(new EventLoop(this)),
    notify_recv_fd_(-1),
    notify_send_fd_(-1),
    next_worker_index_(0),
    is_start_(false){}

SignalingServer::~SignalingServer(){
    if(el_){
        delete el_;
    }

    for(auto worker : workers_){
        if(worker){
            delete worker;
        }
    }
    workers_.clear();
}

int SignalingServer::init(const char* conf_file){
    if(!conf_file){
        RTC_LOG(LS_WARNING) << "signaling server conf_file is null";
        return -1;
    }

    try{
        YAML::Node config = YAML::LoadFile(conf_file);
        RTC_LOG(LS_INFO) << "signaling server options:\n" << config;

        options_.host = config["host"].as<std::string>();
        options_.port = config["port"].as<int>();
        options_.worker_num = config["worker_num"].as<int>();
        options_.connection_timeout = config["connection_timeout"].as<int>();
     
    }catch(YAML::Exception& e){
        RTC_LOG(LS_WARNING) << "catch a YAML exception,line : " << e.mark.line + 1
            << ", column: " << e.mark.column + 1 << ", error : " << e.msg;
        return -1;
    }

    // 用于停止事件循环的单向pipe fd
    int fds[2];
    if(pipe(fds) == -1){
        RTC_LOG(LS_WARNING) << "create pipe failed";
        return -1;
    }
    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];

    pipe_watcher_ = el_->create_io_event(signaling_server_recv_notify,this);
    el_->start_io_event(pipe_watcher_,notify_recv_fd_,EventLoop::READ);

    // 创建tcp server
    listen_fd_ = create_tcp_server(options_.host.c_str(), options_.port);
    if(listen_fd_ < 0){
        RTC_LOG(LS_WARNING) << "create tcp server failed";
        return -1;
    }
    io_watcher_ = el_->create_io_event(accept_new_conn, this);
    el_->start_io_event(io_watcher_,listen_fd_,EventLoop::READ);

    // 创建worker
    for(int i = 0; i < options_.worker_num; ++i){
        if(create_worker(i) != 0){
            return -1;
        }
    }

    return 0;
}

bool SignalingServer::start(){
    if(is_start_){
        RTC_LOG(LS_WARNING) << "signaling server is already started";
        return false;
    }

    is_start_ = true;

    t_ = std::thread([=](){
        RTC_LOG(LS_INFO) << "signaling server event loop run" ;
        el_->start();
        RTC_LOG(LS_INFO) << "signaling server event loop stop" ;
        is_start_ = false;
    });

    return true;
}

void SignalingServer::stop(){
    notify(SignalingServer::QUIT);
}

int SignalingServer::notify(int msg){
    int written = write(notify_send_fd_, &msg, sizeof(msg));
    return written == sizeof(int) ? 0 : -1;
}

void SignalingServer::process_notify(int msg){
    switch(msg){
        case QUIT:
            stop_();
            break;
        default:
            RTC_LOG(LS_WARNING) << "UNKOWN MSG";
            break;
    }
}

void SignalingServer::stop_(){
    if(!is_start_){
        RTC_LOG(LS_WARNING) << "signaling server is not started";
        return;
    }

    el_->delete_io_event(pipe_watcher_);
    el_->delete_io_event(io_watcher_);
    el_->stop();

    close(notify_recv_fd_);
    close(notify_send_fd_);
    close(listen_fd_);

    RTC_LOG(LS_INFO) << "signaling server stop";

    for(auto worker : workers_){
        if(worker){
            worker->stop();
            worker->join();
        }
    }
}

void SignalingServer::join(){
    if(t_.joinable()){
        t_.join();
    }
}

int SignalingServer::create_worker(int worker_id){
    RTC_LOG(LS_INFO) << "signalingserver create worker , worker_id : " << worker_id;

    SignalingWorker* worker = new SignalingWorker(worker_id,options_);

    if(worker->init() != 0){
        return -1;
    }

    if(!worker->start()){
        return -1;
    }

    workers_.push_back(worker);
    return 0;
}

void SignalingServer::dispatch_new_conn(int cfd){
    int index = next_worker_index_;
    next_worker_index_++;
    if(next_worker_index_ >= (int)workers_.size()){
        next_worker_index_ = 0;
    }

     // 分发新连接给worker
    workers_[index]->notify_new_conn(cfd);
}

}