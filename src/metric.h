
#ifndef __RP_METRIC_H__
#define __RP_METRIC_H__

#include <string>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include <map>

namespace rp {

inline int64_t ustime(void) {
    struct timeval tv;
    int64_t ust;

    gettimeofday(&tv, NULL);
    ust = ((int64_t)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

/**
 * Metric
 **/
struct Metric {
    virtual void Clear() {}
    virtual std::string String() { return ""; }
};

/**
 * TimeSumMetric
 **/
struct TimeSumMetric : public Metric {
    int64_t    timeTake;
    int64_t sumTake;
    uint64_t    sum;

public:
    TimeSumMetric() : timeTake(0), sumTake(0), sum(0) {}

public:
    TimeSumMetric& TimingBegin() {
        timeTake = ustime();
        return *this;
    }

    TimeSumMetric& Inc( const uint64_t& count = 1 ) {
        if ( timeTake > 0 ) {
            timeTake = ustime() - timeTake;
            sumTake += timeTake;
        }

        sum += count;
        return *this;
    }

    virtual void Clear() {
        timeTake = 0;
        sumTake = 0;
        sum = 0;
    }

    virtual std::string String() {
        std::stringstream ss;
        ss << "take=" << sumTake/1000 << "ms sum=" << sum;
        return ss.str();
    }
};

/**
 * MetricFactory
 **/
class MetricFactory {
public:
    TimeSumMetric* FetchTimeSum( const std::string& name ) {
        MetricMapType::iterator it = metricMap_.find(name);
        if ( it == metricMap_.end() ) {
            TimeSumMetric* metric = new TimeSumMetric;
            metricMap_.insert( MetricMapType::value_type(name, metric) );
            return metric;
        }

        return dynamic_cast<TimeSumMetric*>(it->second);
    }

    void PrintInfo() {
        for ( MetricMapType::iterator it = metricMap_.begin(); it != metricMap_.end(); it++ ) {
            Metric* metric = it->second;
            printf( "Metric:[%s]%s\n", it->first.c_str(), metric->String().c_str() );

            metric->Clear();
        }
        printf("\n");
    }

private:
    typedef std::map<std::string, Metric*>   MetricMapType;
    MetricMapType   metricMap_;
};

extern MetricFactory* MetricFactoryInstance;

}

#endif
