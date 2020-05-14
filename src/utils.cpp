
#include <stdlib.h>

#include "utils.h"

namespace rp {

int32_t ParseInt32( const Buffer& buffer, int start, int offset ) {
    if ( buffer.Size() == 0 || buffer.Size() <= (std::size_t)start ) { return 0; }
    return atoi( buffer.Data() + start );
}

}
