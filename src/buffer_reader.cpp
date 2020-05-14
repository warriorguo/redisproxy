
#include <string.h>

#include "buffer_reader.h"

namespace rp {

bool BufferReader::Eof() const {
    std::size_t size = buffer_.Size();
    if ( offset_ >= size ) {
        return true;
    }

    return false;
}

Error BufferReader::ReadUntil( Buffer* buffer, const char& ch, int extend ) {
    if ( Eof() ) { return Error::OutOfBound; }

    Buffer tmpBuffer( buffer_ );
    tmpBuffer.Offset( offset_ );

    const char* data = tmpBuffer.Data();
    const char* pch = (const char *)memchr( data, ch, tmpBuffer.Size() );
    if ( pch == NULL ) {
        return Error::NotFound;
    }
    
    std::size_t offset = pch - data + extend;
    Error err = buffer->Append( tmpBuffer, offset );
    if ( !err.None() ) {
        return err;
    }

    offset_ += offset;
    return Error::OK;
}

Error BufferReader::ReadUntil( Buffer* buffer, const std::vector<char>& chs, int extend ) {
    if ( Eof() ) { return Error::OutOfBound; }

    Buffer tmpBuffer( buffer_ );
    tmpBuffer.Offset( offset_ );

    const char* data = tmpBuffer.Data();

    for ( std::size_t i = 0; i < chs.size(); ++i ) {
        const char* pch = (const char *)memchr( data, chs[i], tmpBuffer.Size() );
        if ( pch != NULL ) {
            std::size_t offset = pch - data + extend;
            Error err = buffer->Append( tmpBuffer, offset );
            if ( !err.None() ) {
                return err;
            }

            offset_ += offset;
            return Error::OK;
        }
    }

    /**
    for ( std::size_t i = 0; i < length; i++ ) {
        for ( std::vector<char>::const_iterator it = chs.begin(); it != chs.end(); it++ ) {
            if ( tmpBuffer[i] == *it ) {
                std::size_t offset = i + extend;
                Error err = buffer->Append( data, offset );
                if ( !err.None() ) {
                    return err;
                }

                offset_ += offset;
                return Error::OK;
            }
        }
    }
    */

    return Error::NotFound;
}

Error BufferReader::Read( Buffer* buffer, const std::size_t& length ) {
    if ( Eof() ) { return Error::OutOfBound; }

    std::size_t leftLength = buffer_.Size() - offset_;
    if ( length > leftLength ) {
        return Error::OutOfBound;
    }

    Buffer tmpBuffer( buffer_ );
    tmpBuffer.Offset( offset_ );
    
    Error err = buffer->Append( tmpBuffer, length );
    if ( !err.None() ) {
        return err;
    }

    offset_ += length;
    return Error::OK;
}

Error BufferReader::Next( const std::size_t& length ) {
    if ( Eof() ) { return Error::OutOfBound; }

    std::size_t leftLength = buffer_.Size() - offset_;
    if ( length > leftLength ) {
        return Error::OutOfBound;
    }

    offset_ += length;
    return Error::OK;
}

const char BufferReader::Current() const {
    if ( Eof() ) { return 0; }
    const char* data = buffer_.Data() + offset_;
    return *data;
}

const char BufferReader::Last() const {
    if ( offset_ == 0 ) { return 0; }
    const char* data = buffer_.Data() + offset_ - 1;
    return *data;
}

}
