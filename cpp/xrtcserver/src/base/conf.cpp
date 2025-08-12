
#include <iostream>

#include "base/conf.h"
#include "yaml-cpp/yaml.h"

namespace xrtc {

int load_general_conf(const char* filename, GeneralConf* conf){
    if(!filename || !conf){
        std::cerr << "load_general_conf invalid param" << std::endl;
        return -1;
    }

    conf->log_dir = "./log";
    conf->log_name = "undefined";
    conf->log_level = "info";
    conf->log_to_stderr = false;

    YAML::Node config;
    config = YAML::LoadFile(filename);

    try{
        conf->log_dir = config["log"]["log_dir"].as<std::string>();
        conf->log_name = config["log"]["log_name"].as<std::string>();
        conf->log_level = config["log"]["log_level"].as<std::string>();
        conf->log_to_stderr = config["log"]["log_to_stderr"].as<bool>();
    }catch(YAML::Exception& e){
        std::cerr << "load_general_conf log_dir  exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}


}