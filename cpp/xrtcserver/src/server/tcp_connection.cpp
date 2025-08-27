#include "server/tcp_connection.h"

namespace xrtc{

TcpConnection::TcpConnection(int fd) : cfd(fd){
    
}

TcpConnection::~TcpConnection(){

}

}