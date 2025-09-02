#include <iostream>
#include <signal.h>

#include "base/conf.h"
#include "base/log.h"
#include "server/signaling_server.h"
#include "server/rtc_server.h"

xrtc::GeneralConf* g_general_conf = nullptr;
xrtc::XrtcLog* g_log = nullptr;
xrtc::SignalingServer* g_signaling_server = nullptr;
xrtc::RtcServer* g_rtc_server = nullptr;

int init_general_conf(const char* filename) {
    if(!filename) {
        std::cerr << "init_general_conf invalid param" << std::endl;
        return -1;
    }

    g_general_conf = new xrtc::GeneralConf();
    if(!g_general_conf) {
        std::cerr << "init_general_conf new GeneralConf failed" << std::endl;
        return -1;
    }

    if(xrtc::load_general_conf(filename, g_general_conf) != 0) {
        std::cerr << "init_general_conf load_general_conf failed" << std::endl;
        delete g_general_conf;
        g_general_conf = nullptr;
        return -1;
    }

    return 0;
}

int init_log(std::string log_dir,std::string log_name,std::string log_level){
    g_log = new xrtc::XrtcLog(log_dir,log_name,log_level);

    int ret = g_log->init();
    if(ret != 0){
        std::cerr << "init log error !!" <<std::endl;
        return -1;
    }

    g_log->start();

    return 0;
}

int init_signaling_server() {
    g_signaling_server = new xrtc::SignalingServer();
    int ret = g_signaling_server->init("./conf/signaling_server.yaml");
    if (ret != 0) {
        return -1;
    }

    return 0;
}

int init_rtc_server(){
    g_rtc_server = new xrtc::RtcServer();
    int ret =  g_rtc_server->init("./conf/rtc_server.yaml");
    if (ret != 0) {
        return -1;
    }

    return 0;
}

int main() {
    int ret = init_general_conf("./conf/general.yaml");
    if(ret != 0) {
        return -1;
    }

    ret = init_log(g_general_conf->log_dir,g_general_conf->log_name,g_general_conf->log_level);

    ret = init_signaling_server();
    if(ret != 0){
        return -1;
    }

    ret = init_rtc_server();
    if(ret != 0){
        return -1;
    }

    g_signaling_server->start();

    g_rtc_server->start();

    g_signaling_server->join();

    g_rtc_server->join();

    return 0;
}