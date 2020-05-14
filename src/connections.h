
#ifndef __RP_CONNECTIONS_H__
#define __RP_CONNECTIONS_H__

#include <string>
#include <functional>
#include <list>
#include <map>

#include "buffer.h"
#include "recycle.h"
#include "io.h"
#include "options.h"

namespace rp {

/**
 * ConnectionOptions
 **/
struct ConnectionOptions : public OptionsLoader {
    io::NetIoOptions    NetOpt;

    std::size_t ReadBufferInitSize;
    std::size_t ReadBufferMinSize;
    std::size_t ConnSendBufferCount;
    std::size_t ConnPoolSize;

    std::string name;
    ConnectionOptions( const std::string& n );
    virtual ~ConnectionOptions() {}

    virtual std::string Name() { return name; }
    virtual Error Load( const std::string& key, const std::string& value ) {
        if ( key == "ReadBufferInitSize" ) { ReadBufferInitSize = std::stoi(value); }
        else if ( key == "ReadBufferMinSize" ) { ReadBufferMinSize = std::stoi(value); }
        else if ( key == "ConnSendBufferCount" ) { ConnSendBufferCount = std::stoi(value); }
        else if ( key == "ConnPoolSize" ) { ConnPoolSize = std::stoi(value); }
        else {
            return Error::Unknown;
        }
        return Error::OK;
    }
    virtual std::string String() { return ""; }
};

enum {
    NET_FLAG_CLOSE      = 1 << 1,
    NET_FLAG_RST        = 1 << 2,

    NET_FLAG_WRITE,
    NET_FLAG_READ,
    NET_FLAG_ERROR,
};

enum {
    CONN_SEND_BUFFER_SIZE   = 32
};

class Session;
class ConnectionPool;

/**
 * Connection
 **/
class Connection : public io::Event {
public:
    typedef std::function<Error ( Connection* )>  WriteEventHandlerType;
    typedef std::function<Error ( Connection* )>  ClosedEventHandlerType;
    typedef std::function<Error ( Connection*, Buffer* )>  ReadEventHandlerType;
    typedef std::function<Error ( Connection*, const Error& )>  ErrorEventHandlerType;

public:
    explicit Connection( const ConnectionOptions& opt, ConnectionPool* pool );
    virtual ~Connection() { printf("~Connection()\n"); }

public:
    const io::Addr& GetAddr() const { return addr_; } 

    Error Accept( const io::Addr& addr );
    Error Connect( const io::Addr& addr );
    Error Reconnect();

public:
    bool IsConnected() const { return connected_; }
    bool IsClosed() const;
    bool IsError() const;
    bool IsIdle() const;

public:
    Error WriteToBuffer( const Buffer& b, int flags = 0 ); //NET_FLAG_CLOSE | NET_FLAG_RST

public:
    Error OnWriteEvent( WriteEventHandlerType handler );
    Error OnClosedEvent( ClosedEventHandlerType handler );
    Error OnReadEvent( ReadEventHandlerType handler, Buffer* pb );
    Error OnErrorEvent( ErrorEventHandlerType handler );

public:
    virtual Error OnWritable();
    virtual Error OnReadable();
    virtual void OnClosed();
    virtual void OnError( const Error& err );

public:
    Session* GetSession() { return session_; }
    void SetSession( Session* sess ) { session_ = sess; }

public:
    void SetConnectionPool( ConnectionPool* pool ) { connectionPool_ = pool; }
    ConnectionPool* GetConnectionPool() { return connectionPool_; }

private:
    const ConnectionOptions& opt_;
    io::Addr    addr_;
    int flag_;
    bool connected_;

private:
    Buffer* recvBuffer_;
    Buffer sendBuffer_;

    typedef Recycle<Buffer> BuffersType;
    BuffersType sendBuffers_;

private:
    WriteEventHandlerType    writeEventHander_;
    ReadEventHandlerType    readEventHander_;
    ClosedEventHandlerType  closedEventHandler_;
    ErrorEventHandlerType    errorEventHander_;

private:
    Session*    session_;
    ConnectionPool* connectionPool_;
};


/**
 * ConnectionPool
 **/
class ConnectionPool {
public:
    typedef std::function<Error ( Connection* )>  EventHandlerType;

public:
    io::ContextType Context() const { return ctx_; }

public:
    ConnectionPool( const ConnectionOptions& opt ) : opt_(opt), ctx_(nullptr) {}

public:
    Error Init( io::ContextType ctx ) {
        ctx_ = ctx;
        return Error::OK;
    }

public:
    Error CreateConnection( Connection** pconn );
    void RemoveConnection( Connection* conn );

public:
    void OnCreateConnection( EventHandlerType handler ) { onCreateHander_ = handler; }
    void OnRemoveConnection( EventHandlerType handler ) { onRemoveHander_ = handler; }

public:
    Connection* FetchByAddr( const io::Addr& addr );

private:
    Connection* spawn();
    void recycle( Connection* conn );

private:
    typedef std::list< Connection* >    connection_list_type;
    
private:
    const ConnectionOptions& opt_;
    io::ContextType ctx_;

    EventHandlerType    onCreateHander_;
    EventHandlerType    onRemoveHander_;
};

}

#endif