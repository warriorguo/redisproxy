
#ifndef __RP_LOG_H__
#define __RP_LOG_H__

#include <string>

#include "options.h"

namespace rp {

/**
 * LoggerOptions
 **/
struct LoggerOptions: public OptionsLoader {
    int Level;
    std::string LogFilename;

    virtual ~LoggerOptions() {}
    virtual std::string Name() const { return "log"; }
    virtual Error Load( const std::string& key, const std::string& value ) {
        if ( key == "Level" ) { Level = std::stoi(value); }
        else if ( key == "LogFilename" ) { LogFilename = value; }
        else {
            return Error::Unknown;
        }
        return Error::OK;
    }
    virtual std::string String() { return ""; }
};

class LogEntry;

/**
 * Logger
 **/
class Logger {

public:
    Error Init( const LoggerOptions& opt );

public:
    void Infof(const char* fmt, ...);
    void Warnf(const char* fmt, ...);
    void Errorf(const char* fmt, ...);
    void Fatalf(const char* fmt, ...);

public:
    Logger* Get( const std::string& name );

private:
    LogEntry* entry_;
};

extern Logger glogger;
}

#define LogInfof(fmt, args...)  do{\
    ::rp::glogger.Infof("[%s:%u]" fmt "\n", __FILE__, __LINE__, ##args);\
}while(0);

#define LogWarnf(fmt, args...)  do{\
    ::rp::glogger.Warnf("[%s:%u]" fmt "\n", __FILE__, __LINE__, ##args);\
}while(0);

#define LogErrorf(fmt, args...)  do{\
    ::rp::glogger.Errorf("[%s:%u]" fmt "\n", __FILE__, __LINE__, ##args);\
}while(0);

#define LogFatalf(fmt, args...)  do{\
    ::rp::glogger.Fatalf("[%s:%u]" fmt "\n", __FILE__, __LINE__, ##args);\
}while(0);


#endif
