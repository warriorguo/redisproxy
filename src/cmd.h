
#ifndef __RP_CMD_H__
#define __RP_CMD_H__

#include "buffer_reader.h"

namespace rp {

class CmdParser;

/**
 * Cmd
 * TODO: consider complement a helper function to handle with Connection::WriteToBuffer()
 **/
class Cmd {
public:
    const Buffer* GetCmd() const {
        if ( argv_.empty() ) { 
            return nullptr; 
        }

        return &argv_[0];
    }

    const Buffer* GetArg( int i ) const {
        if ( argv_.size() >= std::size_t(i + 1) ) {
            return &argv_[i + 1];
        }

        return nullptr;
    }

    std::size_t GetArgSize() const {
        std::size_t size = argv_.size();
        if ( size == 0 ) {
            return 0;
        }
        return size - 1; 
    }

public:
    bool Empty() const { return argv_.empty(); }

public:
    Error FormatRESP2( Buffer* buffer ) const;

public:
    void AppendArg( const Buffer& arg ) { argv_.push_back(arg); }
    void Reset() { argv_.clear(); }

private:
    friend class CmdParser;

private:
    /**
     * 0 - cmd
     * 1..N from argv[0]
     **/
    std::vector<Buffer> argv_;
};


namespace impl {

/**
 * InlineParser
 **/
class InlineParser {
public:
    Error HandleRequest( Cmd* cmd, BufferReader& br );
    Error HandleResponse( Buffer* buffer, BufferReader& br );
};

/**
 * MultibulkParser
 **/
class MultibulkParser {
public:
    MultibulkParser() : multibulkLen_(0), multibulkIndex_(0), bulkLen_(0) {}

public:
    Error HandleRequest( Cmd* cmd, BufferReader& br );
    Error HandleResponse( Buffer* buffer, BufferReader& br );
    void Reset() {
        multibulkLen_ = 0;
        multibulkIndex_ = 0;
        bulkLen_ = 0;
    }

private:
    Error readInt32( int32_t* i, BufferReader& br, Buffer *buffer = nullptr );

private:
    int32_t multibulkLen_;
    int32_t multibulkIndex_;

    int32_t bulkLen_;
};

}

/**
 * CmdParser
 **/
class CmdParser {
    enum {
        TYPE_NONE   = 0,
        TYPE_INLINE = 1,
        TYPE_MULTIBULK   = 2,
    };

public:
    CmdParser() : parseType_(TYPE_NONE), inputbr_(inputb_) {}

public:
    Error ParseRequest( Cmd* cmd );
    Error ParseResponse( Buffer* resp );

public:
    void Reset() {
        parseType_ = TYPE_NONE;
        multibulkParser_.Reset();
    }

public:
    Buffer* GetInputBuffer() { return &inputb_; }

private:
    int parseType_;

    impl::InlineParser  inlineParser_;
    impl::MultibulkParser  multibulkParser_;

private:
    Buffer          inputb_;
    BufferReader    inputbr_;
};

}

#endif
