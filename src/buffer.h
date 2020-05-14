
#ifndef __RP_BUFFER_H__
#define __RP_BUFFER_H__

#include <stdint.h>

#include "error.h"

namespace rp {

class Buffer {
public:
    Buffer();
    Buffer( const Buffer& b );
    Buffer( const char* data, std::size_t size );

    ~Buffer();

public:
    void Offset( const int32_t& start, const int32_t& length = 0 );

public:
    Buffer& operator= ( const Buffer& );
    char operator[] ( std::size_t index ) const {
        if ( index >= Size() ) {
            throw Error::OutOfBound;
        }

        return Data()[index];
    }

public:
    char* Tail() { return data_ + size_; }
    const char* Tail() const { return data_ + size_; }
    char* Data() { return data_ + offset_; }
    const char* Data() const { return data_ + offset_; }

public:
    bool Empty() const { return Size() == 0; }
    std::size_t Size() const { return size_ - offset_; }
    std::size_t Offset() const { return offset_; }

    std::size_t FreeSize() const {
        if ( data_ == nullptr ) { return 0; }
        return Capacity() - size_;
    }
    std::size_t Capacity() const {
        if ( data_ == nullptr ) { return 0; }
        return mem::GetRefSize(data_); 
    }

public:
    void Clear();
    
    /**
     * 
     **/
    Error Append( const Buffer& buffer, std::size_t size = 0 );
    Error Append( const char* data, std::size_t size );
    Error AppendCapacity( std::size_t size );
    Error AppendSize( std::size_t size );

private:
    mem::RefType data_;
    std::size_t size_;
    std::size_t offset_;
};

}

#endif
