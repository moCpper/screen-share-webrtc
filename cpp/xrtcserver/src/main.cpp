#include <iostream>

#include "base/conf.h"
#include "base/log.h"

xrtc::GeneralConf* g_general_conf = nullptr;
xrtc::XrtcLog* g_log = nullptr;


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
    return 0;
}

int main() {
    int ret = init_general_conf("./conf/general.yaml");
    if(ret != 0) {
        return -1;
    }

    ret = init_log(g_general_conf->log_dir,g_general_conf->log_name,g_general_conf->log_level);

    // RTC_LOG(LS_VERBOSE) << "test222222222222222222";

    return 0;
}