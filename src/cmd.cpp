
#include <stdio.h>
#include <assert.h>
#include <sstream>

#include "cmd.h"
#include "utils.h"

namespace rp { namespace impl {

static std::vector<char>    kSplitSign; // includes '\n' and '<space>'

struct __init { __init() {
    kSplitSign.push_back(' ');
    kSplitSign.push_back('\n');
}} __init;


Error InlineParser::HandleResponse( Buffer* buf, BufferReader& br ) {
    Error err = br.ReadUntil( buf, '\n', 1 );
    if ( err == Error::NotFound ) {
        return Error::TryAgain;
    }

    return Error::OK;
}

/**
 * Handle from inline format
 **/
Error InlineParser::HandleRequest( Cmd* cmd, BufferReader& br ) {
    Buffer buf;
    Error err = br.ReadUntil( &buf, kSplitSign );

    for ( ; err.None(); err = br.ReadUntil( &buf, kSplitSign ) ) {
        if ( br.Current() == '\n' ) {
            if ( br.Last() == '\r' ) {
                buf.Offset(0, -1);
            }

            cmd->AppendArg( buf );
            br.Next();
                
            return Error::OK;
        } else {
            cmd->AppendArg( buf );
            br.Next();
        }

        buf.Clear();
    }

    /**
     * wait for next coming data
     **/
    if ( err == Error::NotFound ) {
        return Error::TryAgain;
    }

    return err;
}

Error MultibulkParser::HandleResponse( Buffer* pbuf, BufferReader& br ) {
    Error err;

    if ( multibulkLen_ == 0 ) {
        char ch = br.Current();
        if ( ch == '*' ) {
            // read from a new line
            err = readInt32( &multibulkLen_, br, pbuf );
            if ( !err.None() ) {
                return err; 
            }
        } else /*if ( ch == '$' )*/ {
            multibulkLen_ = 1;
        }
    }

    for ( ; multibulkIndex_ < multibulkLen_; ++multibulkIndex_ ) {
        if ( bulkLen_ == 0 ) {
            err = readInt32( &bulkLen_, br, pbuf );
            if ( !err.None() ) { 
                return err; 
            }
            
            if ( bulkLen_ <= 0 ) {
                bulkLen_ = 0;
                continue;
            }
        }

        err = br.Read( pbuf, bulkLen_ + 2 ); // plus endro '\r\n'
        if ( !err.None() ) {
            if ( err == Error::OutOfBound ) {
                return Error::TryAgain;
            }
            return err;
        }

        bulkLen_ = 0;
    }

    return Error::OK;
}

/**
 * Handle from multibulk format
 **/
Error MultibulkParser::HandleRequest( Cmd* cmd, BufferReader& br ) {
    Error err;

    if (multibulkLen_ == 0) {
        // read from a new line
        err = readInt32( &multibulkLen_, br );
        if ( !err.None() ) { return err; }
    }

    for ( ; multibulkIndex_ < multibulkLen_; ++multibulkIndex_ ) {
        if ( bulkLen_ == 0 ) {
            err = readInt32( &bulkLen_, br );
            if ( !err.None() ) { return err; }
        }

        Buffer buf;
        err = br.Read( &buf, bulkLen_ + 2 ); // plus endro '\r\n'
        if ( !err.None() ) {
            if ( err == Error::OutOfBound ) {
                return Error::TryAgain;
            }
            return err;
        }

        buf.Offset(0, -2);
        cmd->AppendArg( buf );

        bulkLen_ = 0;
    }

    return Error::OK;
}

Error MultibulkParser::readInt32( int32_t* i, BufferReader& br, Buffer *pbuf ) {
    Buffer buf;

    // read from a new line
    Error err = br.ReadUntil( &buf, '\n', 1 );
    if ( !err.None() ) {
        if ( err == Error::NotFound ) {
            return Error::TryAgain;
        }
        return err;
    }

    if ( pbuf != nullptr ) {
        pbuf->Append( buf.Data(), buf.Size() );
    }

    // passed '*' or '$' and '\r'
    *i = ParseInt32( buf, 1, -1 );
    return Error::OK;
}

}}

namespace rp {

Error Cmd::FormatRESP2( Buffer* buffer ) const {
    Error err;

    std::stringstream ss;
    ss << "*" << argv_.size() << "\r\n";
    std::string s( ss.str() );
    err = buffer->Append( s.c_str(), s.size() );
    if ( !err.None() ) {
        return err;
    }

    for ( std::size_t i = 0; i < argv_.size(); ++i ) {
        std::stringstream ss;
        ss << "$" << argv_[i].Size() << "\r\n";
        std::string s(ss.str());
        err = buffer->Append( s.c_str(), s.size() );
        if (!err.None()) {
            return err;
        }

        err = buffer->Append( argv_[i].Data(), argv_[i].Size() );
        if ( !err.None() ) {
            return err;
        }

        err = buffer->Append("\r\n", 2);
        if ( !err.None() ) {
            return err;
        }
    }

    return Error::OK;
}

Error CmdParser::ParseRequest( Cmd* cmd ) {
    Error err;
    // No data in buffer
    if ( inputbr_.Eof() ) {
        return Error::TryAgain;
    }

    if ( parseType_ == TYPE_NONE ) {
        char ch = inputbr_.Current();
        if ( ch == '*' ) {
            parseType_ = TYPE_MULTIBULK;
        } else {
            parseType_ = TYPE_INLINE;
        }
    }

    std::size_t currentOffset = inputbr_.Offset();
    if ( parseType_ == TYPE_MULTIBULK ) {
        err = multibulkParser_.HandleRequest( cmd, inputbr_ );
    } else {
        err = inlineParser_.HandleRequest( cmd, inputbr_ );
    }

    /**
     * if the parser accomplished some read, 
     * reset the reader offset trying to save space.
     **/
    if ( currentOffset != inputbr_.Offset() ) {
        inputb_.Offset( inputbr_.Offset() );
        inputbr_.Reset();
    }

    return err;
}

Error CmdParser::ParseResponse( Buffer* resp ) {
    Error err;
    // No data in buffer
    if ( inputbr_.Eof() ) {
        return Error::TryAgain;
    }

    if ( parseType_ == TYPE_NONE ) {
        char ch = inputbr_.Current();
        if ( ch == '*' || ch == '$' ) {
            parseType_ = TYPE_MULTIBULK;
        } else if ( ch == '-' || ch == '+' || ch == ':' ) {
            parseType_ = TYPE_INLINE;
        } else {
            return Error::Unknown;
        }
    }

    std::size_t currentOffset = inputbr_.Offset();
    if ( parseType_ == TYPE_MULTIBULK ) {
        err = multibulkParser_.HandleResponse( resp, inputbr_ );
    } else {
        err = inlineParser_.HandleResponse( resp, inputbr_ );
    }

    if ( currentOffset != inputbr_.Offset() ) {
        inputb_.Offset( inputbr_.Offset() );
        inputbr_.Reset();
    }

    return err;
}

}

#ifdef RP_CMD_TEST
int main() {
    rp::CmdParser parser;
    rp::Cmd cmd;
    rp::Error err;
    rp::Buffer output;

    rp::Buffer* buffer = parser.GetInputBuffer();

    char s1[] = "set key 1";
    err = buffer->Append( s1, sizeof(s1) - 1 );

    parser.ParseRequest( &cmd );
    if ( cmd.GetCmd() != nullptr ) {
        printf( "Cmd:[%d]%s\n", cmd.GetCmd()->Size(), cmd.GetCmd()->Data() );
        for ( std::size_t i = 0; i < cmd.GetArgSize(); i++ ) {
            printf( "Arg:[%d]%s\n", cmd.GetArg(i)->Size(), cmd.GetArg(i)->Data() );
        }
    } else {
        printf( "Cmd:<null>\n" );
    }

    char s2[] = "23\r\n";
    err = buffer->Append( s2, sizeof(s2) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    printf( "input:[%d]%s", buffer->Size(), buffer->Data() );

    err = parser.ParseRequest( &cmd );
    if ( !err.None() ) {
        printf( "Parse failed:%s\n", err.String().c_str() );
        return 1;
    }

    if ( cmd.GetCmd() != nullptr ) {
        printf( "Cmd:[%d]%s\n", cmd.GetCmd()->Size(), cmd.GetCmd()->Data() );
        for ( std::size_t i = 0; i < cmd.GetArgSize(); i++ ) {
            printf( "Arg:[%d]%s\n", cmd.GetArg(i)->Size(), cmd.GetArg(i)->Data() );
        }
    } else {
        printf( "Cmd:<null>\n" );
    }

    parser.Reset();
    cmd.Reset();

    printf("\n\n");

    char s3[] = "*3\r\n$";//*3\r\n$3\r\nset\r\n$5\r\n
    err = buffer->Append( s3, sizeof(s3) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    printf( "input:[%d]%s", buffer->Size(), buffer->Data() );

    err = parser.ParseRequest( &cmd );
    if ( !err.None() ) {
        printf( "1 Parse failed:%s\n", err.String().c_str() );
    }

    char s4[] = "3\r\nse";
    err = buffer->Append( s4, sizeof(s4) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    err = parser.ParseRequest( &cmd );
    if ( !err.None() ) {
        printf( "2 Parse failed:%s\n", err.String().c_str() );
    }

    char s5[] = "t\r\n$5\r\n";
    err = buffer->Append( s5, sizeof(s5) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    err = parser.ParseRequest( &cmd );
    if ( !err.None() ) {
        printf( "3 Parse failed:%s\n", err.String().c_str() );
    }

    char s6[] = "aaaaa\r\n$1\r\n1\r\n";
    err = buffer->Append( s6, sizeof(s6) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    err = parser.ParseRequest( &cmd );
    if ( !err.None() ) {
        printf( "4 Parse failed:%s\n", err.String().c_str() );
    }

    if ( cmd.GetCmd() != nullptr ) {
        printf( "Cmd:[%d]%s\n", cmd.GetCmd()->Size(), cmd.GetCmd()->Data() );
        for ( std::size_t i = 0; i < cmd.GetArgSize(); i++ ) {
            printf( "Arg:[%d]%s\n", cmd.GetArg(i)->Size(), cmd.GetArg(i)->Data() );
        }
    } else {
        printf( "Cmd:<null>\n" );
    }

    parser.Reset();
    cmd.Reset();

    printf("\n\n");

    char s7[] = "+OK\r\n";
    err = buffer->Append( s7, sizeof(s7) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    char s8[] = "$3\r\nxxx\r\n";
    err = buffer->Append( s8, sizeof(s8) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    char s9[] = "*2\r\n$3\r\nxxx\r\n$6\r\nshowme\r\n";
    err = buffer->Append( s9, sizeof(s9) - 1 );
    if ( !err.None() ) {
        printf( "Buffer Append failed:%s\n", err.String().c_str() );
        return 1;
    }

    err = parser.ParseResponse( &output );
    if ( !err.None() ) {
        printf( "1 ParseResponse failed:%s\n", err.String().c_str() );
    } else {
        printf( "1 output:[%d]%s\n", output.Size(), output.Data() );
        parser.Reset();
        cmd.Reset();
    }

    err = parser.ParseResponse( &output );
    if ( !err.None() ) {
        printf( "2 ParseResponse failed:%s\n", err.String().c_str() );
    } else {
        printf( "2 output:[%d]%s\n", output.Size(), output.Data() );
        parser.Reset();
        cmd.Reset();
    }

    err = parser.ParseResponse( &output );
    if ( !err.None() ) {
        printf( "3 ParseResponse failed:%s\n", err.String().c_str() );
    } else {
        printf( "3 output:[%d]%s\n", output.Size(), output.Data() );
        parser.Reset();
        cmd.Reset();
    }

    return 0;
}
#endif
