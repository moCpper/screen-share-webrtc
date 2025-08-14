#include <rtc_base/logging.h>
#include <yaml-cpp/yaml.h>

#include "server/signaling_server.h"
#include "base/socket.h"

namespace xrtc{

SignalingServer::SignalingServer(){

}

SignalingServer::~SignalingServer(){

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

    // 创建tcp server
    listen_fd_ = create_tcp_server(options_.host.c_str(), options_.port);

    return 0;
}

}