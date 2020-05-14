
#ifndef __RP_UPSTREAM_H__
#define __RP_UPSTREAM_H__

#include "connections.h"
#include "recycle.h"
#include "cmd.h"

namespace rp {

namespace cmd {

class Auth;
class Info;
class Cluster;

}

/**
 * UpstreamReader
 **/
struct UpstreamReader {
    virtual uint64_t Identity() const { return 0; }
    virtual void OnServerWrite( const Buffer& buffer ) = 0;
};

/**
 * Upstream
 **/
class Upstream {
public:
    Upstream( const ConnectionOptions& opt ) : 
        opt_(opt), serverConn_(nullptr), connectTimes_(0), auth_(false),
        authCmd_(nullptr), cmdQueue_(opt.ConnSendBufferCount) {}
    ~Upstream();

    /**
     * Init()
     **/
    Error Init( Connection* conn, const std::string& password );
    Error Close();

public:
    Error OnServerClosed( Connection* conn );
    Error OnServerRead( Connection* conn, Buffer* buffer );

public:
    Error PushRequest( const Cmd& cmd, UpstreamReader* reader );

public:
    bool IsAcceptable() const;

private:
    void onAuthCallback( bool ok, const Buffer& cb );

private:
    const ConnectionOptions&    opt_;

    /**
     * link to upstream redis server
     **/
    Connection* serverConn_;
    uint32_t connectTimes_;

    /**
     * 
     **/
    Buffer respBuffer_;
    CmdParser parser_;

private:
    bool    auth_;
    cmd::Auth   *authCmd_;

private:
    struct ReaderPair {
        uint64_t identity;
        UpstreamReader* reader;

        ReaderPair() : identity(0), reader(nullptr) {}
        ReaderPair( UpstreamReader* r, uint64_t id ) : identity(id), reader(r) {}
    };

    typedef Recycle<ReaderPair> CmdQueueType;
    CmdQueueType  cmdQueue_;
};

/**
 * UpstreamPool
 **/
class UpstreamPool {
public:
    UpstreamPool( const ProxyOptions& opt, ConnectionPool* pool ) : 
        opt_(opt), pool_(pool), singular_(nullptr) {}
    ~UpstreamPool();

public:
    Error Init();

public:
    /**
     * check if it is in cluster mode.
     **/
    Error PushRequest( const Cmd& cmd, UpstreamReader* reader );

private:
    const ProxyOptions& opt_;
    ConnectionPool* pool_;

private:
    Upstream* singular_;

};

}

#endif
