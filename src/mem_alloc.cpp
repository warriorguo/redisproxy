
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "mem_alloc.h"

namespace rp { namespace mem {

#define atomicIncr(var,count) __sync_add_and_fetch(var,(count))
#define atomicDecr(var,count) __sync_sub_and_fetch(var,(count))

Type Alloc( uint32_t size ) {
    char* rm = (char *)malloc( size + sizeof(uint32_t) );

    if (rm == nullptr) { return nullptr; }

    *(uint32_t *)(rm) = size;
    return rm + sizeof(uint32_t);
}

Type Realloc( Type m, uint32_t size ) {
    char* rm = m - sizeof(uint32_t);

    rm = (char *)realloc( rm, size + sizeof(uint32_t) );
    if (rm == nullptr) { return nullptr; }

    *(uint32_t *)(rm) = size;
    return rm + sizeof(uint32_t);
}

void Free( Type m ) {
    char* rm = m - sizeof(uint32_t);
    free(rm);
}

uint32_t GetSize( Type m ) {
    char* rm = m - sizeof(uint32_t);
    return *(uint32_t *)(rm);
}

RefType AllocRef( uint32_t size ) {
    char* ref = Alloc( size + sizeof(uint32_t) );
    if (ref == nullptr) { return nullptr; }

    *(uint32_t *)(ref) = 1;
    char* m = ref + sizeof(uint32_t);

    return m;
}

char* ReallocRef( RefType m, uint32_t size ) {
    uint32_t count = GetRefCount( m );
    char* newM = nullptr;

    if ( count <= 1 ) {
        char* ref = m - sizeof(uint32_t);
        ref = Realloc( ref, size + sizeof(uint32_t) );

        newM = ref + sizeof(uint32_t);
    } else {
        DescRef( m );
        newM = AllocRef( size );

        uint32_t copySize = GetSize(m);
        if ( copySize > size ) { copySize = size; }

        memcpy( newM, m, copySize );
    }

    return newM;
}

uint32_t GetRefCount( RefType m ) {
    char* ref = m - sizeof(uint32_t);
    return *(uint32_t *)(ref);
}

uint32_t GetRefSize( RefType m ) {
    char* ref = m - sizeof(uint32_t);
    return GetSize(ref) - sizeof(uint32_t);
}

void IncrRef( RefType m ) {
    char* ref = m - sizeof(uint32_t);
    atomicIncr( (uint32_t *)(ref), 1 );
}

void DescRef( RefType m ) {
    char* ref = m - sizeof(uint32_t);
    if ( atomicDecr( (uint32_t *)(ref), 1 ) == 0 ) {
        Free(ref);
    }
}

}}


#ifdef RP_MEM_TEST
int main() {
    char* ref0 = rp::mem::AllocRef( 100 );
    strcpy( ref0, "show me the money" );

    char *ref1 = ref0;
    rp::mem::IncrRef( ref1 );

    printf( "ref1[%d]:%s\n", rp::mem::GetRef(ref1), ref1 );

    rp::mem::DescRef( ref1 );
    rp::mem::DescRef( ref0 );
}
#endif
