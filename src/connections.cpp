#include <stdio.h>

#include "connections.h"
#include "metric.h"

namespace rp {

Connection* ConnectionPool::spawn() {
    //printf("new conn from %s:%d\n", addr.Host(), addr.Port());
    return new Connection( opt_, this ); 
}

void ConnectionPool::recycle( Connection* conn ) { 
    //printf("close conn from %s:%d\n", conn->GetAddr().Host(), conn->GetAddr().Port());
    delete conn; 
}

Error ConnectionPool::CreateConnection( Connection** pconn ) {
    Connection* conn = spawn();
    if ( onCreateHander_ ) {
        Error err = onCreateHander_( conn );
        if ( !err.None() ) {
            //TODO: make log here
            delete conn;
            return Error::InitFailed;
        }
    }
        
    *pconn = conn;
    return Error::OK;
}

void ConnectionPool::RemoveConnection( Connection* conn ) {
    if ( onRemoveHander_ ) {
        onRemoveHander_( conn );
    }

    recycle( conn );
}

Connection::Connection( const ConnectionOptions& opt, ConnectionPool* pool ) :
    io::Event( pool->Context() ), opt_(opt), flag_(0), connected_(false),
    sendBuffers_(opt.ConnSendBufferCount), connectionPool_(pool) {;}

Error Connection::Accept( const io::Addr& addr ) {
    addr_ = addr;
    connected_ = true;

    return Error::OK;
}

Error Connection::Connect( const io::Addr& addr ) {
    addr_ = addr;
    connected_ = true;

    return io::Connect( this, addr_, opt_.NetOpt );
}

Error Connection::Reconnect() {
    connected_ = true;
    return io::Connect( this, addr_, opt_.NetOpt );
}

Error Connection::OnReadEvent( ReadEventHandlerType handler, Buffer* pb ) {
    readEventHander_ = handler;
    recvBuffer_ = pb;
    return SetReadable( true );
}

Error Connection::OnClosedEvent( ClosedEventHandlerType handler ) {
    closedEventHandler_ = handler;
    return Error::OK;
}

Error Connection::WriteToBuffer( const Buffer& b, int flags ) {
    Error err = SetWritable( true );
    if ( !err.None() ) {
        return err; 
    }

    err = sendBuffer_.Append( b.Data(), b.Size() );
    /**
    err = sendBuffers_.Push( b );
    */
    if ( !err.None() ) {
        return err; 
    }

    if ( NET_FLAG_RST & flags ) {
        flags |= NET_FLAG_CLOSE; 
    }
    flag_ |= flags;

    return Error::OK;
}

Error Connection::OnWritable() {
    if ( sendBuffer_.Size() == 0 ) {
        SetWritable( false );
        return Error::OK;
    }

    //TimeSumMetric* writeDev = MetricFactoryInstance->FetchTimeSum("writedev");

    //writeDev->TimingBegin();
    Error err = Write( &sendBuffer_ );
    //writeDev->Inc();

    bool sentOut = true;
    if ( !err.None() ) {
        if ( err == Error::TryAgain ) {
            sentOut = false;
        } else {
            printf("write failed:%s\n", err.String().c_str());
            return err;
        }
    } else {
        sendBuffer_.Clear();
    }
    /**
    BuffersType::IteratorType it;
    sendBuffers_.FromBegin( &it );
    for( ; !it.Eof(); it.Next() ) {
        Buffer& buf( *it );

        if ( buf.Size() == 0 ) {
            continue;
        }

        writeDev->TimingBegin();
        Error err = Write( &buf );
        writeDev->Inc();

        if ( !err.None() ) {
            if ( err == Error::TryAgain ) {
                sentOut = false;
                break;
            } else {
                printf("write failed:%s\n", err.String().c_str());
                return;
            }
        }        
    }
    sendBuffers_.EraseUntil(it);
    */

    if (writeEventHander_) {
        writeEventHander_( this );
    }

    if ( flag_ & NET_FLAG_CLOSE ) {
        Error err = Close( flag_ );
        if ( !err.None() ) {
            // Log this error
        }

        return Error::Closed;
    }

    if ( sentOut ) {
        SetWritable( false );
    }

    return Error::OK;
}

Error Connection::OnReadable() {
    if ( recvBuffer_ == nullptr ) {
        // Log the unexcepted null ptr and disable the notifier.
        SetReadable( false );
        return Error::OK;
    }

    Error err = Read( recvBuffer_ );
    if ( !err.None() ) {
        if ( err != Error::Full ) { 
            printf("Read failed:%s\n", err.String().c_str());
            return err; 
        }
        
        printf("recv buffer is unexpected full\n");
    }

    if (readEventHander_) {
        readEventHander_( this, recvBuffer_ );
    }

    return Error::OK;
}

void Connection::OnClosed() {
    connected_ = false;

    if (closedEventHandler_) {
        closedEventHandler_( this );
    }

    if ( connectionPool_ != nullptr ) {
        connectionPool_->RemoveConnection( this );
    }
}

void Connection::OnError( const Error& err ) {
    if (errorEventHander_) {
        errorEventHander_( this, err );
    }

    printf("connection: err:%s\n", err.String().c_str());
}

}
