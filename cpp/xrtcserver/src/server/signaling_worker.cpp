#include <unistd.h>
#include <iostream>

#include "xrtcserver_def.h"
#include "base/xhead.h"
#include "server/signaling_worker.h"
#include "server/tcp_connection.h"
#include "server/rtc_server.h"
#include "rtc_base/logging.h"
#include "rtc_base/zmalloc.h"
#include "base/socket.h"

extern xrtc::RtcServer* g_rtc_server;

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
    if(events & EventLoop::WRITE){
        worker->write_reply(fd);
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
        RTC_LOG(LS_INFO) << "SignalingWorker thread run ,worker_id : " << worker_id_;
        el_->start();
        RTC_LOG(LS_INFO) << "SignalingWorker thread stop ,worker_id : " << worker_id_;
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

void SignalingWorker::add_reply(std::shared_ptr<TcpConnection> c,const rtc::Slice& reply){
    c->reply_list.push_back(reply);    
    el_->start_io_event(c->io_watcher_,c->cfd,EventLoop::WRITE);
}

void SignalingWorker::response_server_offer(std::shared_ptr<RtcMsg> msg){
    TcpConnection* c_ptr = static_cast<TcpConnection*>(msg->conn);
    if(!c_ptr){
        RTC_LOG(LS_WARNING) << "tcpconnection is null ";
        return;
    }

    // 若链接以超时？ 先通过cfd查找tcpconnection是否存在
    int fd = msg->fd;
    if(fd <= 0 || (size_t)fd >= conns_.size() || !conns_[fd]){
        RTC_LOG(LS_WARNING) << "response_server_offer invalid fd : " << fd;
        return;
    }

    if(conns_[fd].get() != c_ptr){     // 新的链接，不做处理
        return;
    }

    auto c = conns_[fd];

    xhead_t* xh = reinterpret_cast<xhead_t*>(c->querybuf);
    rtc::Slice header(c->querybuf, XHEAD_SIZE);
    char* buf = (char*)zmalloc(XHEAD_SIZE + MAX_RES_BUF);
    if(!buf){
        RTC_LOG(LS_WARNING) << "zmalloc error ";
        return;
    }

    memcpy(buf, header.data(), XHEAD_SIZE);
    xhead_t* res_xh = reinterpret_cast<xhead_t*>(buf);

    Json::Value res_root;
    res_root["err_no"] = msg->err_no;
    if(msg->err_no != 0){
        res_root["err_msg"] = "process error";
        res_root["offer"] = "";
    }else{
        res_root["err_msg"] = "process success";
        res_root["offer"] = msg->sdp;
    }

    Json::StreamWriterBuilder write_builder;
    write_builder.settings_["indentation"] = "";
    std::string json_data = Json::writeString(write_builder,res_root);
    RTC_LOG(LS_INFO) << "response json data : " << json_data;

    res_xh->body_len = json_data.size();
    snprintf(buf + XHEAD_SIZE,MAX_RES_BUF,"%s",json_data.c_str());

    rtc::Slice reply(buf,XHEAD_SIZE + res_xh->body_len);
    add_reply(c,reply);
}

void SignalingWorker::process_rtc_msg(){
    auto msg = pop_msg();
    if(!msg){
        return;
    }
    switch(msg->cmdno){
        case CMDNO_PUSH:
            response_server_offer(msg);
            break;
        default:
            RTC_LOG(LS_WARNING) << "SignalingWorker unknown rtc msg cmdno : " << msg->cmdno;
            break;
    }
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
        case RTC_MSG:
            process_rtc_msg();
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
        close_conn(c);
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

void SignalingWorker::write_reply(int fd){
    if(fd <= 0 || (size_t)fd >= conns_.size() || !conns_[fd]){
        RTC_LOG(LS_WARNING) << "write_reply invalid fd : " << fd;
        return;
    }  

    auto c = conns_[fd];
    if(!c){return;}

    while(!c->reply_list.empty()){
        rtc::Slice reply = c->reply_list.front();
        int nwritten = sock_write_data(c->cfd, reply.data() + c->cur_resp_pos, reply.size() - c->cur_resp_pos);
        if(nwritten < 0){
            close_conn(c);
            return;
        }else if(nwritten == 0){
            RTC_LOG(LS_WARNING) << "written zero bytes,fd : " << c->cfd;
        }else if((nwritten + c->cur_resp_pos) >= reply.size()){
            // 写入完成
            c->reply_list.pop_front();
            zfree((void*)reply.data());
            c->cur_resp_pos = 0;
            RTC_LOG(LS_INFO) << "write finished, fd : " << c->cfd << "worker id : " << worker_id_;
        }else{
            c->cur_resp_pos += nwritten;
        }
    }

    c->last_interaction = el_->now();
    if(c->reply_list.empty()){
        el_->stop_io_event(c->io_watcher_,c->cfd,EventLoop::WRITE);
        RTC_LOG(LS_INFO) << "stop write event , fd: " << c->cfd << ",worker_id : " << worker_id_; 
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

    const xhead_t* xh = reinterpret_cast<const xhead_t*>(header.data());

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    Json::Value root;
    JSONCPP_STRING err;

    if (!reader->parse(body.data(), body.data() + body.size(), &root, &err)) {
        RTC_LOG(LS_WARNING) << "process_request parse json failed, fd : " << c->cfd << "error is : " << err << "log_id : " << xh->log_id;
        return -1;
    }

    int cmdno;

    try{
        cmdno = root["cmdno"].asInt();

    }catch(Json::Exception& e){
        RTC_LOG(LS_WARNING) << "no cmdno , log_id : " << xh->log_id;
        return -1;
    }

    switch(cmdno){
        case CMDNO_PUSH:
            return process_push(cmdno,c,root,xh->log_id);
            break;
        case CMDNO_PULL:
            // handle pull
            break;
        case CMDNO_ANSWER:
            // handle answer
            break;
        case CMDNO_STOPPUSH:
            // handle stop push
            break;
        case CMDNO_STOPPULL:
            // handle stop pull
            break;
        default:
            RTC_LOG(LS_WARNING) << "process_request unknown cmdno, fd : " << c->cfd << "log_id : " << xh->log_id;
            return -1;
    }

    return 0;
}

int SignalingWorker::process_push(int cmdno,std::shared_ptr<TcpConnection> c,const Json::Value& root,uint32_t log_id){
    RTC_LOG(LS_INFO) << "process_push, fd : " << c->cfd << " log_id : " << log_id;

    uint64_t uid;
    std::string stream_name;
    int audio;
    int video;

    try{
        uid = root["uid"].asUInt64();
        stream_name = root["stream_name"].asString();
        audio = root["audio"].asInt();
        video = root["video"].asInt();

    }catch(Json::Exception& e){
        RTC_LOG(LS_WARNING) << "process_push parse json failed, fd : " << c->cfd << "error is : " << e.what() << "log_id : " << log_id;
        return -1;
    }

    RTC_LOG(LS_INFO) << "process_push, uid : " << uid 
        << ", stream_name : " << stream_name 
        << ", audio : " << audio 
        << ", video : " << video 
        << ", log_id : " << log_id;

    std::shared_ptr<RtcMsg> msg = std::make_shared<RtcMsg>();
    msg->cmdno = cmdno;
    msg->uid = uid;
    msg->stream_name = stream_name;
    msg->audio = audio;
    msg->video = video;
    msg->log_id = log_id;
    msg->worker = this;
    msg->conn = c.get();
    msg->fd = c->cfd;

    return g_rtc_server->send_rtc_msg(msg);
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

int SignalingWorker::send_rtc_msg(std::shared_ptr<RtcMsg> msg){
    if(!msg){
        return -1;
    }

    push_msg(msg);
    return notify(RTC_MSG);
}

void SignalingWorker::push_msg(std::shared_ptr<RtcMsg> msg){
    std::unique_lock<std::mutex> lock(q_msg_mtx_);
    rtc_msg_queue_.push(msg);
}

std::shared_ptr<RtcMsg> SignalingWorker::pop_msg(){
    std::unique_lock<std::mutex> lock(q_msg_mtx_);
    if(rtc_msg_queue_.empty()){
        return nullptr;
    }
    auto msg = rtc_msg_queue_.front();
    rtc_msg_queue_.pop();
    return msg;
}

}