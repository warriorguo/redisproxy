
#ifndef __RP_BUFFERREADER_H__
#define __RP_BUFFERREADER_H__

#include <vector>

#include "buffer.h"

namespace rp {

/**
 * BufferReader
 **/
class BufferReader {
public:
    explicit BufferReader( const Buffer& b ) : buffer_(b), offset_(0) {}

public:
    Error ReadUntil( Buffer* buffer, const char& ch, int extend = 0 );
    Error ReadUntil( Buffer* buffer, const std::vector<char>& chs, int extend = 0 );

    Error Read( Buffer* buffer, const std::size_t& length );

public:
    Error Next( const std::size_t& length = 1 );

public:
    const char Current() const;
    const char Last() const; 

public:
    bool Eof() const;
    std::size_t Offset() const { return offset_; }
    void Reset() { offset_ = 0; }

private:
    const Buffer& buffer_;
    std::size_t offset_;
};

}

#endif
