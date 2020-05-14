
#include <stdarg.h>
#include <stdio.h>

#include "logger.h"
#include "io.h"


namespace rp {
/**
class LogEntry : public io::Event {
public:
    Error Init() {
        std::string logfile( genLogFilename() );
        Error err = io::Create( this, logfile );
        if ( !err.None() ) {
            return err;
        }

        return Error::OK;
    }

private:
    std::string genLogFilename();

public:
    virtual void OnReadable() {}
    virtual void OnWritable() {
        
    }
    
    virtual void OnError( const Error& err ) {
        fprintf( stderr, "[LOG] Unable to write data to log file:%s", err.Message() );
    }
};
*/

Logger glogger;
}

namespace rp {

Error Logger::Init( const LoggerOptions& opt ) {
    return Error::OK;
}

void Logger::Infof(const char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    vfprintf ( stderr, fmt, args );
    va_end (args);
}

void Logger::Warnf(const char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    vfprintf ( stderr, fmt, args );
    va_end (args);
}

void Logger::Errorf(const char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    vfprintf ( stderr, fmt, args );
    va_end (args);
}

void Logger::Fatalf(const char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    vfprintf ( stderr, fmt, args );
    va_end (args);
}

}

