
#ifndef __RP_ERROR_H__
#define __RP_ERROR_H__

#include <string>

#include "mem_alloc.h"

namespace rp {

/**
 * Error
 **/
class Error {
public:
    const static Error OK;
    const static Error Closed;
    const static Error Empty;
    const static Error Full;
    const static Error Eof;

    const static Error TryAgain;
    const static Error InitFailed;

    const static Error Unknown;
    const static Error NotImplemented;

    const static Error Exhausted;
    const static Error OutOfBound;
    const static Error NotFound;

private:
    //mem::RefType   data_;
    int code_;

public:
    Error() : /*data_(nullptr)*/code_(0) {}
    Error( int code, const char* msg = nullptr );
    Error( const Error& err );
    ~Error();

public:
    void Set( int code, const char* msg );
    Error& operator=( const Error& err );

public:
    bool None() const { return code_ == 0; } /*{ return data_ == nullptr; }*/

    int Code() const { return code_; } /*{
        if ( data_ == nullptr ) { return 0; }
        return *( (int *)data_ );
    }*/

    const char* Message() const;
    std::string String() const;

public:
    bool Equal( const Error& err ) const;
    bool operator ==( const Error& err ) const { return Equal( err ); }
    bool operator !=( const Error& err ) const { return !Equal( err ); }
};

}

#endif