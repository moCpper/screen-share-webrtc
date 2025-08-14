#ifndef __SIGNALING_SERVER_H__
#define __SIGNALING_SERVER_H__

#include <string>

namespace xrtc{

struct SignalingServerOptions{
   std::string host;
   int port;
   int worker_num;
   int connection_timeout;
};

class SignalingServer{
public:
   SignalingServer();
   ~SignalingServer();

   int init(const char* conf_file);
private:
   SignalingServerOptions options_;
   
   int listen_fd_;
};

}

#endif