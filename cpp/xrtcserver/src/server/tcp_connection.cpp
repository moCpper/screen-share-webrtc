#include "server/tcp_connection.h"
#include "base/xhead.h"

namespace xrtc{

TcpConnection::TcpConnection(int fd) : 
    cfd(fd),  
    io_watcher_(nullptr),
    bytes_expected(XHEAD_SIZE), 
    bytes_processed(0),
    querybuf(sdsempty()) {

}

TcpConnection::~TcpConnection(){}

}