
#include "server.h"
#include "connections.h"
#include "cmd.h"
#include "metric.h"

namespace rp {
MetricFactory* MetricFactoryInstance = nullptr;

Error Server::newClientConnection( Connection** pconn ) {
    Connection* conn = nullptr;
    Error err = connPool_.CreateConnection( &conn );
    if ( !err.None() ) {
        return err; 
    }

    Session* sess;
    err = sessPool_.CreateSession( &sess, conn );
    if ( !err.None() ) {
        return err; 
    }

    err = sess->OnNewClientConnection( conn );
    if ( !err.None() ) { 
        return err; 
    }

    *pconn = conn;
    return Error::OK;
}

Error Server::Init() {
    MetricFactoryInstance = new MetricFactory;
    lastMetricUpdate_ = ustime() / 1000;

    io::NetIoOptions& opt(serverOpt_.ClientOpt->NetOpt);
    {
        using namespace std::placeholders;
        opt.NewConnectionHandler = std::bind( &Server::newClientConnection, this, _1 );
    }

    Error err = io::Init( &listenEvt_.context, opt );
    if ( !err.None() ) {
        return err;
    }

    err = connPool_.Init( listenEvt_.context );
    if ( !err.None() ) {
        return err;
    }

    io::Addr addr(serverOpt_.BindHost.c_str(), serverOpt_.BindPort);
    err = io::Listen( &listenEvt_, addr, serverOpt_.ClientOpt->NetOpt );
    if ( !err.None() ) {
        return err;
    }

    err = upstreamPool_.Init();
    if ( !err.None() ) {
        return err;
    }

    return Error::OK;
}

Error Server::RunOnce() {
    //TimeSumMetric* pollMetric = MetricFactoryInstance->FetchTimeSum( "poll" );

    io::NetIoOptions& opt(serverOpt_.ClientOpt->NetOpt);

    //pollMetric->TimingBegin();
    Error err = io::PollOnce( listenEvt_.context, &listenEvt_, opt );
    //pollMetric->Inc();

    if ( !err.None() ) {
        // handle the error
        return err;
    }

    int64_t now = ustime() / 1000;
    if ( now -  lastMetricUpdate_ >= 5000) {
        lastMetricUpdate_ = now;
        //MetricFactoryInstance->PrintInfo();
    }

    return Error::OK;
}

Error Server::Run() {
    while( true ) {
        Error err = RunOnce();
        if ( !err.None() ) { return err; }
    }

    return Error::OK;
}

}
