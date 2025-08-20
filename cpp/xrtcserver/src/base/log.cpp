#include <iostream>
#include <sys/stat.h>

#include "base/log.h"

namespace xrtc{

XrtcLog::XrtcLog(std::string log_dir,std::string log_name,std::string log_level) :
    log_dir_(std::move(log_dir)),
    log_name_(std::move(log_name)),
    log_level_(std::move(log_level)),
    log_file_(log_dir_ + "/" + log_name_ + ".log"),
    log_file_wf_(log_dir_ + "/" + log_name_ + ".log.wf"){}
    
XrtcLog::~XrtcLog(){
    stop();
}

static rtc::LoggingSeverity get_log_serverity(std::string level){
    if("verbose" == level){
        return rtc::LS_VERBOSE;
    }else if("info" == level){
        return rtc::LS_INFO;
    }else if("warning" == level){
        return rtc::LS_WARNING;
    }else if("error" == level){
        return rtc::LS_ERROR;
    }else if("none" == level){
        return rtc::LS_NONE;
    }
    return rtc::LS_NONE;
}

int XrtcLog::init(){
    rtc::LogMessage::ConfigureLogging("thread tstamp");
    rtc::LogMessage::SetLogPathPrefix("/src");
    rtc::LogMessage::AddLogToStream(this,rtc::LS_VERBOSE);

    int ret = mkdir(log_dir_.c_str(),0755);
    if(ret != 0 && errno != EEXIST){
        std::cerr << "mkdir error ! " << std::endl;
        return -1;
    }

     out_file_.open(log_file_, std::ios::app);
    if (!out_file_.is_open()) {
        fprintf(stderr, "open log_file[%s] failed\n", log_file_.c_str());
        return -1;
    }
    
    out_file_wf_.open(log_file_wf_, std::ios::app);
    if (!out_file_wf_.is_open()) {
        fprintf(stderr, "open log_file_wf[%s] failed\n", log_file_wf_.c_str());
        return -1;
    }

    return 0;
}

bool XrtcLog::start() {
    if (running_) {
        fprintf(stderr, "log thread already running\n");
        return false;
    }

    running_ = true;

    t_ = std::thread([=]() {
        struct stat stat_data;
        std::stringstream ss;

        while (running_) {
            // 检查日志文件是否被删除或者移动
            if (stat(log_file_.c_str(), &stat_data) < 0) {
                out_file_.close();
                out_file_.open(log_file_, std::ios::app);
            }

            if (stat(log_file_wf_.c_str(), &stat_data) < 0) {
                out_file_wf_.close();
                out_file_wf_.open(log_file_wf_, std::ios::app);
            }
           
            bool write_log = false;
            {
                std::unique_lock<std::mutex> lock(log_mtx_);
                if (!log_queue_.empty()) {
                    write_log = true;
                    while (!log_queue_.empty()) {
                        ss << log_queue_.front();
                        log_queue_.pop();
                    }
                }
            }

            if (write_log) {
                out_file_ << ss.str();
                out_file_.flush();
            }
            
            ss.str("");

            bool write_log_wf = false;
            {
                std::unique_lock<std::mutex> lock(log_wf_mtx_);
                if (!log_queue_wf_.empty()) {
                    write_log_wf = true;
                    while (!log_queue_wf_.empty()) {
                        ss << log_queue_wf_.front();
                        log_queue_wf_.pop();
                    }
                }
            }

            if (write_log_wf) {
                out_file_wf_ << ss.str();
                out_file_wf_.flush();
            }
          
            ss.str("");

            // std::cout << "ffffffffffffffffffffff" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

       out_file_.close();
       out_file_wf_.close();
    });
    
    return true;
}

void XrtcLog::stop(){
    running_ = false;
    if (t_.joinable()) {
        t_.join();
    }
}

void XrtcLog::join(){
    if (t_.joinable()) {
        t_.join();
    }
}

void XrtcLog::set_log_to_stderr(bool on){
    rtc::LogMessage::SetLogToStderr(on);
}

// 宏RTC_LOG会callback到这里
void XrtcLog::OnLogMessage(const std::string& message,rtc::LoggingSeverity serverity){
    if (serverity >= rtc::LS_WARNING) {
        std::unique_lock<std::mutex> lock(log_wf_mtx_);
        log_queue_wf_.push(message);
    } else {
        std::unique_lock<std::mutex> lock(log_mtx_);
        log_queue_.push(message);
    }
}

void XrtcLog::OnLogMessage(const std::string& message) {

}

}