#include <rtc_base/logging.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "base/socket.h" 

namespace xrtc{

static int generic_accept(int sock, struct sockaddr* addr, socklen_t* addrlen) {
    int fd = -1;
    while(1){
        fd = accept(sock,addr,addrlen);
        if (fd < 0) {
            if(errno == EINTR){    // 系统调用被临时中断
                continue;
            }else{
                RTC_LOG(LS_WARNING) << "accept error, errno : " << errno
                     << ", error : " << strerror(errno);
                return -1;
            }
        }
        break;
    }
    return fd;
}


int create_tcp_server(const char* addr, int port){
    if(!addr || port <= 0){
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        RTC_LOG(LS_WARNING) << "create socket error,errno : " << errno
            << ", error :" << strerror(errno);
        return -1;
    }

    // 设置SO_REUSEADDR
    int opt = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        RTC_LOG(LS_WARNING) << "setsockopt error, errno : " << errno
            << ", error : " << strerror(errno);
        close(sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(inet_pton(AF_INET, addr, &server_addr.sin_addr) == 0){
        RTC_LOG(LS_WARNING) << "inet_pton error, addr : " << addr
            << ", error : " << strerror(errno);
        close(sockfd);
        return -1;
    }

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        RTC_LOG(LS_WARNING) << "bind error, errno : " << errno
            << ", error : " << strerror(errno);
        close(sockfd);
        return -1;
    }

    if(listen(sockfd, 4095) < 0){
        RTC_LOG(LS_WARNING) << "listen error, errno : " << errno
            << ", error : " << strerror(errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int tcp_accept(int sock,char* host,int* port){
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    int fd = generic_accept(sock,(struct sockaddr*)&sa,&salen);
    if(fd == -1){
        return -1;
    }

    if(host){
        strcpy(host,inet_ntoa(sa.sin_addr));
    }

    if(port){
        *port = ntohs(sa.sin_port);
    }

    return fd;
}

}