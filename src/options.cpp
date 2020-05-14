
#include "options.h"
#include "upstream.h"
#include "connections.h"
#include "logger.h"

namespace rp {

SingularOptions::SingularOptions() {
    UpstreamHost = "127.0.0.1";
    UpstreamPort = 6300;
}

ProxyOptions::ProxyOptions() {
    LoggerOpt = new LoggerOptions();
    ClientOpt = new ConnectionOptions("client");
    UpstreamOpt = new ConnectionOptions("upstream");
    SingularOpt = new SingularOptions();

    BindHost = "0.0.0.0";
    BindPort = 9877;

    ClusterMode = false;
}

ProxyOptions::~ProxyOptions() {
    delete LoggerOpt;
    delete ClientOpt;
    delete UpstreamOpt;
    delete SingularOpt;
}

ConnectionOptions::ConnectionOptions(const std::string& n) : name(n) {
    ReadBufferInitSize = 1024 * 16;
    ReadBufferMinSize = 1024;
    ConnSendBufferCount = 10000;
    ConnPoolSize = 768;
}

namespace io {

NetIoOptions::NetIoOptions() {
    ListenBacklog = 1024;
    SocketNoDelay = true;
    SocketNonBlock = true;
    SocketKeepAlive = 0;
}

}

}
