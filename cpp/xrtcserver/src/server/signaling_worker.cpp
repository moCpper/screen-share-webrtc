#include <unistd.h>
#include <iostream>

#include "base/xhead.h"
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

static void conn_time_cb(EventLoop* el,TimerWatcher* w,void* data){
    SignalingWorker* worker = static_cast<SignalingWorker*>(el->owner());

    int cfd = *static_cast<int*>(data);

    worker->process_timeout(cfd);
}

SignalingWorker::SignalingWorker(int worker_id,const SignalingServerOptions& options) 
    : worker_id_(worker_id),
    is_start_(false),
    el_(new EventLoop(this)),
    notify_recv_fd_(-1),
    notify_send_fd_(-1),
    options_(options){}

SignalingWorker::~SignalingWorker(){
    for(auto c : conns_){
        if(c){
            close_conn(c);
        }
    }

    conns_.clear();

    if(el_){
        delete el_;
        el_ = nullptr;
    }
}

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

    c->timer_watcher_ = el_->create_timer(conn_time_cb,&c->cfd,true);       // true 重复定时
    el_->start_timer(c->timer_watcher_,100000); // 100ms

    c->last_interaction = el_->now();

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

    c->last_interaction = el_->now();

    RTC_LOG(LS_INFO) << "sock read data len : " << nread;

    if(nread == -1){
        return;
    }else if(nread > 0){
        sdsIncrLen(c->querybuf, nread);
    }

    int ret = process_query_buffer(c);
    if(ret != 0){
        close_conn(c);
        return;
    }
}

void SignalingWorker::close_conn(std::shared_ptr<TcpConnection> c){
    close(c->cfd);
    remove_conn(c);

    RTC_LOG(LS_INFO) << "SignalingWorker " << worker_id_ 
        << " close connection, cfd : " << c->cfd;
}

void SignalingWorker::remove_conn(std::shared_ptr<TcpConnection> c){
    el_->delete_timer(c->timer_watcher_);
    el_->delete_io_event(c->io_watcher_);
    conns_[c->cfd].reset();
}

int SignalingWorker::process_query_buffer(std::shared_ptr<TcpConnection> c){
    if(!c){
        RTC_LOG(LS_WARNING) << "process_query_buffer invalid connection";
        return -1;
    }

    while(sdslen(c->querybuf) >= c->bytes_processed + c->bytes_expected) {
       xhead_t* head = reinterpret_cast<xhead_t*>(c->querybuf);
       if(c->current_state == TcpConnection::STATE_HEAD){
           if(XHEAD_MAGIC_NUM != head->magic_num){
               RTC_LOG(LS_WARNING) << "process_query_buffer invalid magic number";
               return -1;
           }

           c->current_state = TcpConnection::STATE_BODY;
           c->bytes_processed += XHEAD_SIZE;
           c->bytes_expected = head->body_len;

       }else{
           rtc::Slice header(c->querybuf, XHEAD_SIZE);
           rtc::Slice body(c->querybuf + XHEAD_SIZE, head->body_len);
           int ret = process_request(c,header,body);
           if(ret != 0){
               return -1;
           }

           // 短链接处理
           c->bytes_processed = 65535;
       }
    }

    return 0;
}

int SignalingWorker::process_request(std::shared_ptr<TcpConnection> c,const rtc::Slice& header,const rtc::Slice& body){
    RTC_LOG(LS_INFO) << "receive body :" << body.data();

    return 0;
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

void SignalingWorker::process_timeout(int cfd){
    if(el_->now() - conns_[cfd]->last_interaction > options_.connection_timeout){
            RTC_LOG(LS_INFO) << "connection timeout fd :" << cfd;
        close_conn(conns_[cfd]);
    }
}

}