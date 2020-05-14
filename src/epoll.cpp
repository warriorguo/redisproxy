
#define RP_USE_EPOLL

#ifdef RP_USE_EPOLL

#include <assert.h>
#include <sys/epoll.h>
#include <string.h>

#include "io.h"
#include "error.h"
#include "connections.h"
#include "logger.h"
#include "metric.h"

namespace rp { namespace io {

struct epollState {
    int epfd;

    epoll_event *events;
    int eventsLength;
};

/**
 * Init
 **/
Error Init( ContextType* context, const NetIoOptions& opt ) {
    epollState* es = new epollState;
    es->epfd = epoll_create( 1024 );

    if ( es->epfd == -1 ) {
        delete es;
        return Error::InitFailed;
    }

    es->eventsLength = 100;
    es->events = new epoll_event[ es->eventsLength ];

    *context = static_cast<ContextType>(es);
    return Error::OK;
}

Error Deinit( ContextType context ) {
    epollState* es = static_cast<epollState*>(context);
    if ( es != nullptr ) {
        delete es;
    }
    
    return Error::OK;
}

/**
 * PollOnce()
 **/
Error PollOnce( ContextType context, Event* listenEvt, const NetIoOptions& opt ) {
    epollState* es = static_cast<epollState*>(context);

    //TimeSumMetric* acceptMetric = MetricFactoryInstance->FetchTimeSum( "accept" );
    //TimeSumMetric* readMetric = MetricFactoryInstance->FetchTimeSum( "read" );
    //TimeSumMetric* writeMetric = MetricFactoryInstance->FetchTimeSum( "write" );
    //TimeSumMetric* epollwaitMetric = MetricFactoryInstance->FetchTimeSum( "epollwait" );

    Event::HandleWriteEvents();

    //epollwaitMetric->TimingBegin();
    int numevents = epoll_wait( es->epfd, es->events, es->eventsLength, 10 );
    //epollwaitMetric->Inc();

    if ( numevents > 0 ) {
        for ( int i = 0; i < numevents; i++ ) {
            epoll_event& evData( es->events[i] );
            
            if ( evData.data.ptr == listenEvt ) {
                //acceptMetric->TimingBegin();
                Error err = Accept( listenEvt, opt );
                //acceptMetric->Inc();

                if ( !err.None() ) {
                    LogErrorf( "Accept() failed:%s", err.Message() );
                }
            } else {
                Event* evt = static_cast<Event*>(evData.data.ptr);
                assert( evt != nullptr );

                if ( evData.events & (EPOLLHUP | EPOLLERR) ) {
                    evt->OnError( Error(evData.events, "epoll events") );
                    evt->Close();
                } else {
                    if ( evData.events & EPOLLIN ) {
                        //readMetric->TimingBegin();
                        Error err = evt->OnReadable();
                        //readMetric->Inc();

                        if ( !err.None() ) {
                            // log it and continue
                            continue;
                        }
                    }
                    
                    if ( evData.events & EPOLLOUT ) {
                        //writeMetric->TimingBegin();
                        Error err = evt->OnWritable();
                        //writeMetric->Inc();

                        if ( !err.None() ) {
                            // log it and continue
                            continue;
                        }
                    }
                }
            }
        }
    }

    return Error::OK;
}

/**
 * Attach
 **/
Error Event::Attach() {
    epoll_event ev;

    if ( fd == -1 ) { 
        return Error::InitFailed; 
    }
    
    if ( events == 0 ) { 
        return Error::OK; 
    }

    ev.events = events;
    ev.data.ptr = this;

    epollState* es = static_cast<epollState*>(context);
    if( epoll_ctl(es->epfd, EPOLL_CTL_ADD, fd, &ev) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

Error Event::RemoveNotify() {
    epollState* es = static_cast<epollState*>(context);
    if( epoll_ctl(es->epfd, EPOLL_CTL_DEL, fd, NULL) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    events = 0;
    return Error::OK;
}

Error Event::SetWritable( bool flag ) {
    if ( flag ) {
        if ( !(events & EPOLLOUT) ) {
            events |= EPOLLOUT;
            writeListPos = writeEvents.insert( writeEvents.end(), this );
        }
    } else {
        if ( events & EPOLLOUT ) {
            events &= ~EPOLLOUT;
            writeEvents.erase( writeListPos );
        }
    }

    return Error::OK;
    //return operateNotify( EPOLLOUT, flag );
}

Error Event::SetReadable( bool flag ) {
    return operateNotify( EPOLLIN, flag );
}

Error Event::operateNotify( int newEvents, bool flag ) {
    epoll_event evData;

    int op = EPOLL_CTL_MOD;
    if ( events == 0 ) {
        op = EPOLL_CTL_ADD;
    }

    if (flag) {
        if ( events & newEvents ) {
            return Error::OK;
        }
        events |= newEvents;
    } else {
        if ( !(events & newEvents) ) {
            return Error::OK;
        }
        events &= ~newEvents;
    }

    if ( fd == -1 ) {
        // would effect when Attached
        return Error::OK;
    }

    if ( events == 0 ) {
        op = EPOLL_CTL_DEL;
    }

    evData.events = events;
    evData.data.ptr = static_cast<void *>(this);

    epollState* es = static_cast<epollState*>(context);
    if( epoll_ctl(es->epfd, op, fd, &evData) == -1 ) {
        return Error( errno, strerror(errno) );
    }

    return Error::OK;
}

}}

#endif
