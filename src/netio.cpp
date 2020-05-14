
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "io.h"
#include "connections.h"
#include "metric.h"

namespace rp{ namespace io{

static Error SetCloseNoWait( int fd ) {
    linger stLinger;
    stLinger.l_onoff = 1;
    stLinger.l_linger = 0;

    if( setsockopt(fd, SOL_SOCKET, SO_LINGER, &stLinger, sizeof(stLinger)) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

static Error SetNonblock( int fd ) {
    int flags = fcntl(fd, F_GETFL, 0);
    if ( flags == -1 ) {
        return Error( errno, strerror(errno) );
    }

    if ( fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

static Error SetReuseAddr( int fd ) {
    int optval = 1;
    if ( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

static Error SetNoDelay( int fd, int optval = 1 ) {
    if ( setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)) == -1 ) 
    {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

static Error SetKeepAlive( int fd, int interval ) {
    int val = 1;
    if ( setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    val = interval;
    if ( setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0 ) {
        return Error( errno, strerror(errno) );
    }

    val = interval/3;
    if ( val == 0 ) { val = 1; }
    if ( setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0 ) {
        return Error( errno, strerror(errno) );
    }

    val = 3;
    if ( setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

static Error ConvertAddrInfo( Addr* addr, const sockaddr_storage& sa ) {
    addr->family = sa.ss_family;

    if ( sa.ss_family == AF_INET ) {
        const sockaddr_in *s = (const sockaddr_in *)&sa;
        inet_ntop( AF_INET,(void*)&(s->sin_addr), addr->ip, sizeof(addr->ip) );
        addr->port = ntohs( s->sin_port );
    } else {
        const sockaddr_in6 *s = (const sockaddr_in6 *)&sa;
        inet_ntop( AF_INET6,(void*)&(s->sin6_addr), addr->ip, sizeof(addr->ip) );
        addr->port = ntohs( s->sin6_port );
    }

    return Error::OK;
}

/**
 * GetAddrInfo
 **/
static Error GetAddrInfo( const Addr& addr, addrinfo* holder ) {
    addrinfo hints, *info;
    int rv;
    char buf[33];

    memset( &hints, 0, sizeof(hints) );
    hints.ai_family = addr.Family();
    hints.ai_socktype = SOCK_STREAM;

    sprintf( buf, "%d", addr.port );
    if ( (rv = getaddrinfo(addr.ip, buf, &hints, &info)) != 0 ) {
        return Error( rv, gai_strerror(rv) );
    }
        
    memcpy( holder, info, sizeof(addrinfo) );
    freeaddrinfo(info);
    
    return Error::OK;
}

/**
 * Listen
 **/
Error Listen( Event* evt, const Addr& addr, const NetIoOptions& opt ) {
    addrinfo holder;
    Error err = GetAddrInfo( addr, &holder );
    if ( !err.None() ) {
        return err; 
    }

    int s = socket(holder.ai_family, holder.ai_socktype, holder.ai_protocol);
    if ( s == -1 ) {
        return Error( errno, strerror(errno) );
    }

    err = SetNonblock( s );
    if ( !err.None() ) {
        close( s );
        return err; 
    }

    err = SetReuseAddr( s );
    if ( !err.None() ) {
        close( s );
        return err; 
    }

    if ( bind(s, holder.ai_addr, holder.ai_addrlen) == -1 ) {
        Error err( errno, strerror(errno) );
        close( s );
        return err;
    }

    if ( listen(s, opt.ListenBacklog) == -1 ) {
        Error err( errno, strerror(errno) );
        close( s );
        return err;
    }

    evt->fd = s;
    evt->events = EPOLLIN;

    return evt->Attach();
}

/**
 * Connect
 **/
Error Connect( Event* evt, const Addr& addr, const NetIoOptions& opt ) {
    addrinfo holder;
    Error err = GetAddrInfo( addr, &holder );
    if ( !err.None() ) {
        return err; 
    }

    int s = socket(holder.ai_family, holder.ai_socktype, holder.ai_protocol);
    if ( s < 0 ) {
        return Error( errno, strerror(errno) );
    }

    if ( opt.SocketNoDelay ) {
        err = SetNoDelay(s);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    } else {
        printf("unset SocketNoDelay of s:%d", s);
    }
    
    if ( opt.SocketNonBlock ) {
        err = SetNonblock(s);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    } else {
        printf("unset SocketNonBlock of s:%d", s);
    }

    if ( opt.SocketKeepAlive > 0 ) {
        err = SetKeepAlive(s, opt.SocketKeepAlive);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    }

    if( connect(s, holder.ai_addr, holder.ai_addrlen) == -1 ) {
        if( errno != EINPROGRESS ) {
            Error err( errno, strerror(errno) );

            close( s );
            return err;
        }
    }

    evt->fd = s;
    return evt->Attach();
}

/**
 * Accept
 **/
Error Accept( Event* listenEvt, const NetIoOptions& opt ) {
    int s;
    sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    while( true ) {
        s = accept( listenEvt->fd, reinterpret_cast<sockaddr *>(&sa), &salen );
        if ( s == -1 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                return Error( errno, strerror(errno) );
            }
        }
        break;
    }

    Error err;
    if ( opt.SocketNoDelay ) {
        err = SetNoDelay(s);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    }
    
    if ( opt.SocketNonBlock ) {
        err = SetNonblock(s);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    }

    if ( opt.SocketKeepAlive > 0 ) {
        err = SetKeepAlive(s, opt.SocketKeepAlive);
        if ( !err.None() ) {
            close(s);
            return err;
        }
    }

    Addr addr;
    err = ConvertAddrInfo( &addr, sa );
    if ( !err.None() ) {
        close(s);
        return err;
    }

    Connection* conn;
    err = opt.NewConnectionHandler( &conn );
    if ( !err.None() ) {
        close(s);
        return err;
    }

    conn->fd = s;
    conn->context = listenEvt->context;

    err = conn->Accept( addr );
    if ( !err.None() ) {
        conn->Close();
        return err;
    }

    err = conn->Attach();
    if ( !err.None() ) {
        conn->Close();
        return err;
    }

    return err;
}

/**
 * static method
 **/
Event::EventListType Event::writeEvents;
Error Event::HandleWriteEvents() {
    for ( EventListType::iterator it = writeEvents.begin(); it != writeEvents.end(); ) {
        Event* evt( *it ); ++it;
        
        Error err = evt->OnWritable();
        if ( !err.None() ) {

        }
    }

    return Error::OK;
}

/**
 * Write
 **/
Error Event::Write( Buffer* buffer ) {
    //TimeSumMetric* writeAction = MetricFactoryInstance->FetchTimeSum("writeact");

    //writeAction->TimingBegin();
    int nwrite = write( fd, buffer->Data(), buffer->Size() );
    //writeAction->Inc();

    if ( nwrite < 0 ) {
        if ( errno == EAGAIN ) {
            return Error::TryAgain;
        }

        SetWritable( false );
        Error err( errno, strerror(errno) );
        //OnError( err );

        //Close();
        return err;
    } else if ( nwrite == 0 ) {
        return Error::TryAgain;
    }

    // TimeSumMetric* wbMetric = MetricFactoryInstance->FetchTimeSum("writebytes");
    // wbMetric->Inc( nwrite );
    if ( std::size_t(nwrite) < buffer->Size() ) {
        if ( nwrite > 0 ) { buffer->Offset(nwrite); }
        return Error::TryAgain;
    }
    
    return Error::OK;
}

/**
 * Read
 **/
Error Event::Read( Buffer* buffer ) {
    std::size_t freeSize = buffer->FreeSize();
    if ( freeSize == 0 ) {
        SetReadable(false);
        return Error::Full;
    }


    int nread = read( fd, buffer->Tail(), freeSize );
    if ( nread < 0 ) {
        if ( errno == EAGAIN || errno == EINTR ) {
            return Error::TryAgain;
        }

        Error err( errno, strerror(errno) );
        OnError( err );

        Close();
        return err;
    } else if ( nread == 0 ) {
        Close();
        return Error::Eof;
    }

    // TimeSumMetric* rbMetric = MetricFactoryInstance->FetchTimeSum("readbytes");
    // rbMetric->Inc( nread );

    return buffer->AppendSize( (std::size_t)nread );
}

/**
 * Close
 **/
Error Event::Close( int flags ) {
    Error err = RemoveNotify();
    if ( flags & NET_FLAG_RST ) {
        SetCloseNoWait(fd);
    }

    close(fd);

    OnClosed();
    return err;
}

}}
