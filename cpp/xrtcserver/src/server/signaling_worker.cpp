#include <unistd.h>
#include <iostream>

#include "server/signaling_worker.h"
#include "server/tcp_connection.h"
#include "rtc_base/logging.h"
#include "base/socket.h"

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

static void conn_io_cb(EventLoop* /*el*/,IOWatcher* /*w*/,int fd,
    int events,void* data){

    SignalingWorker* worker = static_cast<SignalingWorker*>(data);

    if(events & EventLoop::READ){
        worker->read_query(fd);
    }

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
    
    if(cfd < 0){
        RTC_LOG(LS_WARNING) << "connect fd error ! cfd : " << cfd;
    }

    sock_setnonblock(cfd);
    sock_setnodelay(cfd);

    std::shared_ptr<TcpConnection> c{std::make_shared<TcpConnection>(cfd)};
    sock_peer_to_str(cfd,c->ip,&c->port);

    c->io_watcher_ = el_->create_io_event(conn_io_cb,this);
    el_->start_io_event(c->io_watcher_,cfd,EventLoop::READ);

    if(cfd >= conns_.size()){
        conns_.resize(cfd + 1);
    }

    conns_[cfd] = c;     // 保存连接

}

void SignalingWorker::read_query(int cfd){
    RTC_LOG(LS_INFO) << "SignalingWorker " << worker_id_ 
        << " read query, cfd : " << cfd;
    
    if(cfd < 0 || (size_t)cfd >= conns_.size() || !conns_[cfd]){
        RTC_LOG(LS_WARNING) << "read_query invalid cfd : " << cfd;
        return;
    }

    std::shared_ptr<TcpConnection> c = conns_[cfd];

    int nread = 0;
    int read_len = c->bytes_expected;    // XHEAD_SIZE
    int qb_len = sdslen(c->querybuf);
    
    c->querybuf = sdsMakeRoomFor(c->querybuf, read_len);

    nread = sock_read_data(cfd,c->querybuf + qb_len, read_len);
    
    RTC_LOG(LS_INFO) << "sock read data len : " << nread;

    if(nread == -1){
        return;
    }else if(nread > 0){
        sdsIncrLen(c->querybuf, nread);
    }
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