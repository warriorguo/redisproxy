
#ifndef __RP_SESSION_H__
#define __RP_SESSION_H__

#include <vector>

#include "connections.h"
#include "upstream.h"
#include "recycle.h"
#include "cmd.h"

namespace rp {

#define NULLID 0

/**
 * Session
 **/
class Session : public UpstreamReader {
public:
    Session( const ConnectionOptions& opt ) :
        clientOpt_(opt), id_(NULLID), clientConn_(nullptr), 
        connectionPool_(nullptr), upstreamPool_(nullptr) {}

    virtual ~Session() {}

public:
    void SetId( uint64_t id ) { id_ = id; }
    uint64_t GetId() const { return id_; }
    virtual uint64_t Identity() const { return GetId(); }

public:
    void SetConnectionPool( ConnectionPool* pool ) { connectionPool_ = pool; }
    void SetUpstreamPool( UpstreamPool* pool ) { upstreamPool_ = pool; }

public:
    Error OnNewClientConnection( Connection* conn );
    Error OnClientRead( Connection* conn, Buffer* buffer );

    virtual void OnServerWrite( const Buffer& buffer );

private:
    const ConnectionOptions&    clientOpt_;

    uint64_t    id_;

    Connection* clientConn_;
    ConnectionPool* connectionPool_;
    UpstreamPool*   upstreamPool_;

    /**
     * 
     **/
    Cmd currentCmd_;
    CmdParser parser_;

    typedef std::vector<Connection*> UpstreamListType;
    UpstreamListType    upstreamList_;
};

/**
 * SessionPool
 **/
class SessionPool {
public:
    SessionPool( const ProxyOptions& opt, UpstreamPool* pool ) : 
        opt_(opt), idCounter_(0), upstreamPool_(pool) {}

public:
    Error CreateSession( Session** psess, Connection* conn ) {
        Session* sess = new Session( *opt_.ClientOpt );

        sess->SetId( ++idCounter_ );
        sess->SetUpstreamPool( upstreamPool_ );
        sess->SetConnectionPool( conn->GetConnectionPool() );
        conn->SetSession( sess );

        *psess = sess;
        return Error::OK;
    }

    void RemoveSession( Session* sess ) {
        delete sess;
    }

private:
    const ProxyOptions& opt_;
    uint64_t idCounter_;

    UpstreamPool*    upstreamPool_;
};

}

#endif
