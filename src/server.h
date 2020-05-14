
#ifndef __RP_SERVER_H__
#define __RP_SERVER_H__

#include "io.h"
#include "connections.h"
#include "session.h"
#include "options.h"
#include "metric.h"

namespace rp {

/**
 * Server
 **/
class Server {
public:
    Server() : serverOpt_(), listenEvt_(nullptr),
        connPool_(*serverOpt_.ClientOpt), upstreamPool_(serverOpt_, &connPool_), 
        sessPool_(serverOpt_, &upstreamPool_) {}

public:
    Error Init();
    Error RunOnce();
    Error Run();

private:
    Error newClientConnection( Connection** pconn );

private:
    ProxyOptions    serverOpt_;
    io::Event   listenEvt_;

    ConnectionPool  connPool_;
    UpstreamPool    upstreamPool_;
    SessionPool sessPool_;

    int64_t lastMetricUpdate_;
};

}

#endif
