
#ifndef __RP_MALLOC_H__
#define __RP_MALLOC_H__

#include <stdint.h>

namespace rp { namespace mem {

typedef char* Type;

Type Alloc( uint32_t size );
Type Realloc( Type m, uint32_t size );
uint32_t GetSize( Type m );
void Free( Type m );

/**
 * 
 **/
typedef char* RefType;

RefType AllocRef( uint32_t size );
RefType ReallocRef( RefType m, uint32_t size );

void IncrRef( RefType m );
void DescRef( RefType m );
uint32_t GetRefCount( RefType m );
uint32_t GetRefSize( RefType m );

}}

#endif
