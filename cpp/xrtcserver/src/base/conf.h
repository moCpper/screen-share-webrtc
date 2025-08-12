#ifndef __BASE_CONF_H__
#define __BASE_CONF_H__

#include <string>

namespace xrtc {

struct GeneralConf {
    std::string log_dir = "./log";
    std::string log_name = "xrtcserver";
    std::string log_level = "info";
    bool log_to_stderr = true;
};

int load_general_conf(const char* file, GeneralConf* conf);

}




#endif //__BASE_CONF_H__