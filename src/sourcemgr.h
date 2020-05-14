
#ifndef __RP_SOURCEMGR_H__
#define __RP_SOURCEMGR_H__

#include "connections.h"

namespace rp {

/**
 * Manage the source
 * Try to send "info" at first
 * If the source run in cluster mode send "cluster nodes" to get all running nodes
 **/
class SourceManager {
public:
    Error Init( const std::vector<addr_type>& sources );

private:
    ConnectionPool*             pool_;
};

}

#endif
