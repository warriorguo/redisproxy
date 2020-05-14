
#ifndef __RP_OPTIONS_H__
#define __RP_OPTIONS_H__

#include <string>

#include "error.h"
#include "buffer.h"

namespace rp {

class Logger;
class LoggerOptions;
class ConnectionOptions;

/**
 * interface of ConfigLoader
 **/
struct OptionsLoader {
    virtual ~OptionsLoader() {}
    virtual std::string Name() const { return ""; }
    virtual Error Load( const std::string& key, const std::string& value ) { return Error::NotImplemented; }
    // return a string with key & value
    virtual std::string String() { return ""; }
};


/**
 * Singular
 **/
struct SingularOptions : public OptionsLoader {
    std::string Password;
    std::string UpstreamHost;
    int UpstreamPort;

    SingularOptions();
    virtual ~SingularOptions() {}
    virtual std::string Name() const { return "singular"; }
    virtual Error Load( const std::string& key, const std::string& value ) {
        if ( key == "Password" ) { Password = value; }
        else if ( key == "UpstreamHost" ) { UpstreamHost = value; }
        else if ( key == "UpstreamPort" ) { UpstreamPort = std::stoi(value); }
        else {
            return Error::Unknown;
        }
        return Error::OK;
    }
    virtual std::string String() { return ""; }
};

/**
 * ProxyConfig
 **/
struct ProxyOptions : public OptionsLoader {
    LoggerOptions*   LoggerOpt;
    ConnectionOptions*   ClientOpt;
    ConnectionOptions*   UpstreamOpt;

    std::string BindHost;
    int BindPort;

    bool ClusterMode;
    SingularOptions*    SingularOpt;

    ProxyOptions();
    virtual ~ProxyOptions();
    virtual Error Load( const std::string& key, const std::string& value ) {
        if ( key == "BindHost" ) { BindHost = value; }
        else if ( key == "ClusterMode" ) { ClusterMode = std::stoi(value); }
        else if ( key == "BindPort" ) { BindPort = std::stoi(value); }
        else {
            return Error::Unknown;
        }
        return Error::OK;
    }
    virtual std::string String() { return ""; }
};


namespace impl {

/**
 * 
 **/
Error LoadFromBuffer( const Buffer& buffer );
Error LoadFromFile( const std::string& filename );
Error LoadFromEtcd();

}

}

#endif
