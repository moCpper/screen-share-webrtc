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
    
XrtcLog::~XrtcLog(){}

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

    return 0;
}

bool XrtcLog::start(){
    if(running_){
        std::cerr << "log thread running !!" << std::endl;
        return false;
    }

    running_ = true;

    t_ = std::thread([=](){
        struct stat stat_data;

        while(running_){
            // 检查日志是否移动或删除
            

            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    });
}

void XrtcLog::end(){

}

void XrtcLog::join(){

}

void XrtcLog::set_log_to_stderr(bool on){
    rtc::LogMessage::SetLogToStderr(on);
}

// 宏RTC_LOG会callback到这里
void XrtcLog::OnLogMessage(const std::string& message,rtc::LoggingSeverity serverity){
    if(serverity >= rtc::LS_WARNING){
        std::unique_lock<std::mutex> lock(log_wf_mtx_);
        lo