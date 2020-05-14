
#include <string.h>
#include <assert.h>

#include "buffer.h"
#include "mem_alloc.h"

namespace rp {

Buffer::Buffer() : data_(nullptr), size_(0), offset_(0) {}

Buffer::Buffer( const Buffer& b ) : data_(nullptr) {
    *this = b;
    /**
    if ( b.data_ != nullptr ) {
        data_ = mem::AllocRef( size_ );
        memcpy( data_, b.data_, size_ );
    } else {
        data_ = nullptr;
    }
    */
}

Buffer::Buffer( const char* data, std::size_t size ) {
    size_ = size;
    offset_ = 0;
    if ( size > 0 ) {
        data_ = mem::AllocRef( size_ );
        memcpy( data_, data, size_ );
    } else {
        data_ = nullptr;
    }
}

Buffer::~Buffer() {
    if ( data_ != nullptr ) {
        mem::DescRef( data_ );
    }
}

Buffer& Buffer::operator= ( const Buffer& b ) {
    if ( data_ != nullptr ) {
        mem::DescRef( data_ );
    }

    size_ = b.size_;
    offset_ = b.offset_;
    
    data_ = const_cast<char *>(b.data_);

    if ( data_ != nullptr ) {
        mem::IncrRef( data_ );
    }
    /**
    if ( b.data_ != nullptr ) {
        data_ = mem::AllocRef( size_ );
        memcpy( data_, b.data_, size_ );
    } else {
        data_ = nullptr;
    }
    */

    return *this;
}

void Buffer::Clear() {
    size_ = 0;
    offset_ = 0;
        
    if ( data_ != nullptr && mem::GetRefCount(data_) > 1 ) {
        mem::DescRef( data_ );
        data_ = nullptr;
    }
}

void Buffer::Offset( const int32_t& start, const int32_t& length ) {
    offset_ += std::size_t(start);
    std::size_t newSize = 0;

    if ( length > 0 ) {
        newSize = offset_ + std::size_t(length);
    } else if ( size_ > std::size_t(-length) ) {
        newSize = size_ - std::size_t(-length);
    }

    if ( newSize < size_ ) { size_ = newSize; }
    if ( offset_ > size_ ) { offset_ = size_; }
}

Error Buffer::AppendSize( std::size_t offset ) {
    std::size_t size = size_ + offset;
    if ( size > Capacity() ) {
        return Error::OutOfBound;
    }

    size_ = size;
    return Error::OK;
}

Error Buffer::AppendCapacity( std::size_t size ) {
    std::size_t capacity = Capacity();
    std::size_t needCapacity = size + size_;

    if ( needCapacity > capacity ) {
        if (data_ != nullptr) {
            if ( mem::GetRefCount(data_) == 1 && offset_ >= needCapacity - capacity ) {
                memcpy( data_, Data(), Size() );

                needCapacity -= offset_;
                size_ -= offset_;
                offset_ = 0;
            } else {
                assert( Size() <=  needCapacity - offset_ );

                mem::RefType data = mem::AllocRef( needCapacity - offset_ );
                if ( data == nullptr ) {
                    return Error::Exhausted;
                }

                memcpy( data, Data(), Size() );

                mem::DescRef( data_ );
                data_ = data;
                size_ -= offset_;
                offset_ = 0;
            }
        } else {
            data_ = mem::AllocRef( needCapacity );
            if ( data_ == nullptr ) {
                return Error::Exhausted;
            }

            size_ = 0;
            offset_ = 0;
        }
    }

    assert( Capacity() >= needCapacity );
    return Error::OK;
}

Error Buffer::Append( const Buffer& buffer, std::size_t size ) {
    std::size_t bufferSize = buffer.Size();
    if ( size == 0 ) {
        size = bufferSize;
    } else if ( size > bufferSize ) {
        return Error::OutOfBound;
    }

    if ( data_ == nullptr ) {
        data_ = const_cast<mem::RefType>( buffer.data_ );
        mem::IncrRef( data_ );

        offset_ = buffer.offset_;
        size_ = offset_ + size;
    } else if ( data_ == buffer.data_ ) {
        size_ += size;
    } else {
        return Append( buffer.Data(), size );
    }

    return Error::OK;
}

Error Buffer::Append( const char* data, std::size_t size ) {
    std::size_t needCapacity = size + size_;
    std::size_t capacity = Capacity();

    if ( needCapacity > capacity ) {
        Error err = AppendCapacity( size );
        if ( !err.None() ) {
            return err;
        }
    }

    memcpy( data_ + size_, data, size );
    Error err = AppendSize( size );

    return err;
}

}
