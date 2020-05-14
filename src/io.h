
#ifndef __RP_NETIO_H__
#define __RP_NETIO_H__

#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <list>

#include "options.h"
#include "buffer.h"
#include "logger.h"

namespace rp {

class Connection;

namespace io {

struct Event;
struct Addr;

typedef std::function<Error (Connection**)> NewConnectionHandlerType;

/**
 * NetIoOptions
 **/
struct NetIoOptions : public OptionsLoader {
    NewConnectionHandlerType    NewConnectionHandler;

    int ListenBacklog;
    bool SocketNoDelay;
    bool SocketNonBlock;
    int SocketKeepAlive;

    NetIoOptions();
    virtual ~NetIoOptions() {}
    virtual Error Load( const std::string& key, const std::string& value ) {
        if ( key == "ListenBacklog" ) { ListenBacklog = std::stoi(value); }
        else if ( key == "SocketNoDelay" ) { SocketNoDelay = std::stoi(value); }
        else if ( key == "SocketNonBlock" ) { SocketNonBlock = std::stoi(value); }
        else if ( key == "SocketKeepAlive" ) { SocketKeepAlive = std::stoi(value); }
        else {
            return Error::Unknown;
        }
        return Error::OK;
    }
    virtual std::string String() { return ""; }
};

}}

namespace rp { namespace io {

/**
 * Addr
 **/
struct Addr {
    char ip[INET6_ADDRSTRLEN];
    int port;
    int family;

public:
    Addr() : port(0), family(AF_UNSPEC) {
        ip[0] = 0;
    }
    Addr( const char* rip, int rport ) : family(AF_UNSPEC) {
        strcpy( ip, rip );
        port = rport;
    }

    int Family() const { return family; }

    const char* Host() const { return ip; }
    int Port() const { return port; }
};

}}

/**
 * for network & file system
 **/
namespace rp { namespace io {

Error Listen( Event* evt, const Addr& addr, const NetIoOptions& opt );
Error Connect( Event* evt, const Addr& addr, const NetIoOptions& opt );
Error Accept( Event* listenEvt, const NetIoOptions& opt );

Error Open( Event* evt, const std::string& filename );
Error Create( Event* evt, const std::string& filename );

}}

/**
 * 
 **/
namespace rp { namespace io {


typedef void* ContextType;
/**
 * 
 **/
Error Init( ContextType* context, const NetIoOptions& opt );
Error PollOnce( ContextType context, Event* listenEvt, const NetIoOptions& opt );
Error Deinit( ContextType context );

/**
 * Event
 **/
struct Event {
    ContextType context;
    int fd;
    int events;

    typedef std::list<Event*> EventListType;
    EventListType::iterator writeListPos;
    static EventListType writeEvents;
public:
    static Error HandleWriteEvents();

public:
    Event( ContextType ctx ) : context(ctx), fd(-1), events(0) {}

    virtual ~Event() {
        if ( fd != -1 ) {
            RemoveNotify();
        }
    }

public:
    Error SetWritable( bool flag );
    Error SetReadable( bool flag );

public:
    Error Attach();

public:
    Error Write( Buffer* buffer );
    Error Read( Buffer* buffer );
    Error Close( int flags = 0 );
    Error RemoveNotify();

private:
    Error operateNotify( int newEvents, bool flag );

public:
    virtual Error OnWritable() { return Error::NotImplemented; }
    virtual Error OnReadable() { return Error::NotImplemented; }
    virtual void OnClosed() {}
    virtual void OnError( const Error& err ) {}
};

}}

#endif
