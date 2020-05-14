
#include <stdio.h>

#include "server.h"

int main() {
    rp::Server server;
    rp::Error err;

    err = server.Init();
    if ( !err.None() ) {
        printf( "Server Init failed:%s\n", err.String().c_str() );
        return 1;
    }

    err = server.Run();
    if ( !err.None() ) {
        printf( "Server Run failed:%s\n", err.String().c_str() );
        return 1;
    }

    return 0;
}
