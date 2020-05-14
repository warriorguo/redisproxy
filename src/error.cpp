
#include <string.h>
#include <sstream>

#include "error.h"
#include "mem_alloc.h"

namespace rp {

const Error Error::OK;
const Error Error::Closed(-10, "Closed");
const Error Error::Empty(-100, "Empty");
const Error Error::Full(-101, "Full");
const Error Error::Eof(-1, "Eof");

const Error Error::TryAgain(-303, "TryAgain");
const Error Error::InitFailed(-500, "InitFailed");

const Error Error::NotImplemented(-405, "NotImplemented");
const Error Error::Unknown(-503, "Unknown");

const Error Error::Exhausted(-502, "Exhausted");
const Error Error::OutOfBound(-504, "OutOfBound");
const Error Error::NotFound(-404, "NotFound");

}

namespace rp {

Error::Error( int code, const char* msg ) /*: data_(nullptr)*/ {
    Set( code, msg );
}

Error::Error( const Error& err ) /*: data_(nullptr)*/ {
    /**
    data_ = const_cast<char *>(err.data_);
    if ( data_ != nullptr ) {
        mem::IncrRef( data_ );
    }
    */
    *this = err;
}

Error::~Error() {
    /**
    if ( data_ != nullptr ) {
        mem::DescRef( data_ );
        data_ = nullptr;
    }
    */
}

Error& Error::operator=( const Error& err ) {
    code_ = err.code_;
    /**
    if ( data_ != nullptr ) {
        mem::DescRef( data_ );
    }

    
    data_ = const_cast<char *>(err.data_);
    if ( data_ != nullptr ) {
        mem::IncrRef( data_ );
    }
    */
    /**
    Set( err.Code(), err.Message() );
    */

    return *this;
}

void Error::Set( int code, const char* msg ) {
    code_ = code;
    /**
    if ( data_ != nullptr ) {
        mem::DescRef( data_ );
        data_ = nullptr;
    }

    if ( msg == nullptr ) { msg = "<nullptr>"; }

    if ( code != 0 ) {
        int length = 0;
        
        if ( msg != nullptr ) {
            length = strlen( msg ) + 1;
        }

        mem::RefType s = mem::AllocRef( uint32_t(length) + sizeof(int) );
        *( (int *)(s) ) = code;

        if ( length > 0 ) {
            strcpy( s + sizeof(int) , msg );
        }
        
        data_ = s;
    }
    */
}

bool Error::Equal( const Error& err ) const {
    return Code() == err.Code();
}

std::string Error::String() const {
    std::stringstream ss;
    ss << "[" << Code() << "]";
    
    const char* msg = Message();
    if ( msg != nullptr ) { ss << msg; }

    return ss.str();
}

const char* Error::Message() const {
    /**
    if ( data_ == nullptr ) { return nullptr; }
    if ( mem::GetRefSize(data_) == sizeof(int) ) {
        return nullptr;
    }

    return data_ + sizeof(int);
    */
   return nullptr;
}

}
