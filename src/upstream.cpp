
#include <functional>

#include "upstream.h"
#include "session.h"

namespace rp {

namespace cmd {

/**
 * Auth
 **/
class Auth : public UpstreamReader {
public:
    typedef std::function<void (bool, const Buffer&)>    CallbackHandlerType;
public:
    Auth( const std::string& pw, CallbackHandlerType handler ) : callbackHandler_(handler) {
        static Buffer auth( "auth", sizeof("auth") - 1 );

        cmd_.AppendArg( auth );
        cmd_.AppendArg( Buffer(pw.c_str(), pw.size()) );
    }
    virtual ~Auth() {}

public:
    const Cmd& GetCmd() const { return cmd_; }

public:
    virtual void OnServerWrite( const Buffer& buffer ) {
        if ( !buffer.Empty() ) {
            const char* resp = buffer.Data();
            callbackHandler_( resp[0] == '+', buffer );
        } else {
            callbackHandler_( false, buffer );
        }
    }

private:
    Cmd cmd_;
    CallbackHandlerType callbackHandler_;
};

/**
class Info : public UpstreamReader {
public:
};

class Cluster : public UpstreamReader {
public:
    const static Buffer Info;
    const static Buffer Nodes;

public:
    explicit Cluster( const Buffer& subCmd );
    
public:
    virtual Error OnServerWrite( const Buffer& buffer );
};
*/
}

UpstreamPool::~UpstreamPool() {
    if ( singular_ != nullptr ) {
        delete singular_;
    }
}

Error UpstreamPool::Init() {
    if ( opt_.ClusterMode ) {
        return Error::NotImplemented;
    }

    Error err;
    Connection* singularConn;
    io::Addr addr( opt_.SingularOpt->UpstreamHost.c_str(), opt_.SingularOpt->UpstreamPort );

    err = pool_->CreateConnection( &singularConn );
    if ( !err.None() ) {
        return err;
    }

    err = singularConn->Connect( addr );
    if ( !err.None() ) {
        return err;
    }

    singular_ = new Upstream( *opt_.UpstreamOpt );
    return singular_->Init( singularConn, opt_.SingularOpt->Password );
}

Error UpstreamPool::PushRequest( const Cmd& cmd, UpstreamReader* reader ) {
    if ( opt_.ClusterMode ) {
        // cmd.GetArg(0);
        return Error::NotImplemented;
    } else {
        if ( !singular_->IsAcceptable() ) {
            printf("!IsAcceptable()\n");
            return Error::TryAgain;
        }

        return singular_->PushRequest( cmd, reader );
    }

    return Error::OK;
}

Upstream::~Upstream() {
    if ( serverConn_ != nullptr ) {
        delete serverConn_;
        serverConn_ = nullptr;
    }
}

bool Upstream::IsAcceptable() const {
    // uninitally
    if ( serverConn_ == nullptr ) {
        return false;
    }
    // unconnected yet
    if ( !serverConn_->IsConnected() ) {
        return false;
    }
    // unauth yet
    if ( !auth_ ) {
        return false;
    }

    return true;
}

void Upstream::onAuthCallback( bool ok, const Buffer& cb ) {
    auth_ = ok;

    if ( authCmd_ != nullptr ) {
        delete authCmd_;
        authCmd_ = nullptr;
    }
}

Error Upstream::Init( Connection* conn, const std::string& password ) {
    if ( serverConn_ != nullptr ) {
        return Error::InitFailed;
    }

    if ( authCmd_ != nullptr ) {
        // the auth cmd was in process
        return Error::TryAgain;
    }
    
    serverConn_ = conn;
    connectTimes_++;

    Buffer* buffer = parser_.GetInputBuffer();
    Error err = buffer->AppendCapacity( opt_.ReadBufferInitSize );
    if ( !err.None() ) {
        return err;
    }

    {
        using namespace std::placeholders;
        err = serverConn_->OnReadEvent( std::bind( &Upstream::OnServerRead, this, _1, _2 ), buffer );
        if ( !err.None() ) {
            return err;
        }

        err = serverConn_->OnClosedEvent( std::bind( &Upstream::OnServerClosed, this, _1 ) );
        if ( !err.None() ) {
            return err;
        }

        if ( !password.empty() ) {
            authCmd_ = new cmd::Auth( password, std::bind( &Upstream::onAuthCallback, this, _1, _2 ) );
            
            err = PushRequest( authCmd_->GetCmd(), authCmd_ );
            if ( !err.None() ) {
                return err;
            }
        } else {
            auth_ = true;
        }
    }

    return Error::OK;
}

Error Upstream::OnServerClosed( Connection* conn ) {
    connectTimes_++;
    //TODO: check if the connectTimes overcame threshold.
    return serverConn_->Reconnect();
}

Error Upstream::OnServerRead( Connection* conn, Buffer* buffer ) {
    assert( conn == serverConn_ );

    while( !buffer->Empty() ) {
        Error err = parser_.ParseResponse( &respBuffer_ );
        if ( !err.None() ) {
            printf("ParseResponse failed:%s\n", err.String().c_str());
            if ( err == Error::TryAgain ) {
                break;
            }

            // log this error
            // and reconnect to server
            return err;
        }

        ReaderPair pair;
        err = cmdQueue_.Pop( &pair );
        if ( !err.None() ) {
            // expected bug occur, log as crisical and return
            return err;
        }

        // check if the session was valid
        if ( pair.reader->Identity() == pair.identity ) {
            pair.reader->OnServerWrite( respBuffer_ );
        }
        
        parser_.Reset();
        respBuffer_.Clear();
    }

    std::size_t freeSize = buffer->FreeSize();
    if ( freeSize < opt_.ReadBufferMinSize ) {
        Error err = buffer->AppendCapacity( opt_.ReadBufferMinSize );
        if ( !err.None() ) {
            return err;
        }
    }

    return Error::OK;
}

Error Upstream::PushRequest( const Cmd& cmd, UpstreamReader* reader ) {
    Error err;
    if ( !serverConn_->IsConnected() ) {
        return Error::TryAgain;
    }

    err = cmdQueue_.Push( ReaderPair(reader, reader->Identity()) );
    if ( !err.None() ) {
        if ( err == Error::Full ) {
            return Error::TryAgain;
        }
        printf("!cmdQueue_Push()\n");
        return err;
    }

    Buffer buffer;
    err = buffer.AppendCapacity( 4 * 1024 );
    if ( !err.None() ) {
        return err;
    }

    err = cmd.FormatRESP2( &buffer );
    if ( !err.None() ) {
        return err;
    }

    //printf("Push[%d]%s\n====\n", buffer.Size(), buffer.Data());

    err = serverConn_->WriteToBuffer( buffer );
    return err;
}

}
