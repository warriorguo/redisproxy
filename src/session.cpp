
#include <functional>

#include "session.h"
#include "stdio.h"

namespace rp {


void Session::OnServerWrite( const Buffer& buffer ) {
    if ( clientConn_->IsConnected() ) {
        clientConn_->WriteToBuffer( buffer );
        clientConn_->SetReadable( true );
    }
}

Error Session::OnClientRead( Connection* conn, Buffer* buffer ) {
    assert( conn == clientConn_ );

    if ( !currentCmd_.Empty() ) {
        Error err = upstreamPool_->PushRequest( currentCmd_, this );
        if ( !err.None() ) {
            if ( err == Error::TryAgain ) {
                // setup an metric here
                conn->SetReadable( false );
                return Error::OK;
            }

            return err;
        }

        currentCmd_.Reset();
    }

    while( !buffer->Empty() ) {
        Error err = parser_.ParseRequest( &currentCmd_ );
        if ( !err.None() ) {
            if ( err == Error::TryAgain ) {
                break;
            }

            // log this error
            std::string s = err.String();
            Error werr = conn->WriteToBuffer( Buffer( s.c_str(), s.size() ), NET_FLAG_CLOSE );
            if ( !werr.None() ) {
                printf("werr:%s\n", werr.String().c_str());
            }

            return err;
        }

        err = upstreamPool_->PushRequest( currentCmd_, this );
        if ( !err.None() ) {
            printf("PushRequest failed:%s\n", err.String().c_str());
            if ( err == Error::TryAgain ) {
                // setup an metric here
                conn->SetReadable( false );
                return Error::OK;
            }

            return err;
        }

        /**
        const Buffer* buffer = currentCmd_.GetCmd();
        if ( buffer == nullptr ) {
            printf( "cmd:<null>\n" );
        } else {
            printf( "cmd:%s\n", buffer->Data() );
        }
        conn->WriteToBuffer( OK );
        */
        
        parser_.Reset();
        currentCmd_.Reset();
    }

    std::size_t freeSize = buffer->FreeSize();
    if ( freeSize < clientOpt_.ReadBufferMinSize ) {
        Error err = buffer->AppendCapacity( clientOpt_.ReadBufferMinSize );
        if ( !err.None() ) {
            return err;
        }
    }

    return Error::OK;
}

Error Session::OnNewClientConnection( Connection* conn ) {
    clientConn_ = conn;

    Buffer* buffer = parser_.GetInputBuffer();
    Error err = buffer->AppendCapacity( clientOpt_.ReadBufferInitSize );
    if ( !err.None() ) {
        return err;
    }

    {
        using namespace std::placeholders;
        err = conn->OnReadEvent( std::bind( &Session::OnClientRead, this, _1, _2 ), buffer );
    }

    return err;
}

}
