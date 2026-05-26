#ifndef DBMS_PAIN_METRICS_COLLECTOR_H
#define DBMS_PAIN_METRICS_COLLECTOR_H

#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>
#include <memory>
#include "QueryType.h"
#include "RollingCounter.h"
#include "ExponentialMovingAverage.h"

class MetricsCollector {
public:
    static MetricsCollector& instance();
    
    void recordQuery(QueryType type, std::chrono::microseconds latency, bool success = true);
    
    double getCurrentRps(QueryType type) const;
    double getAvgRps(QueryType type) const;
    size_t getMaxRps(QueryType type) const;
    double getAvgLatencyMs(QueryType type) const;
    size_t getErrorCount(QueryType type) const;
    double getErrorRate(QueryType type) const;
    
    std::string getStats() const;
    void reset();
    
private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    struct QueryMetrics {
        RollingCounter rps;
        RollingCounter rps10min;
        RollingCounter errorRate;
        ExponentialMovingAverage avgLatency;
        
        QueryMetrics()
            : rps(std::chrono::seconds(1))
            , rps10min(std::chrono::seconds(600))
            , errorRate(std::chrono::seconds(60))
            , avgLatency(0.1) {}
    };
    
    QueryMetrics& getMetrics(QueryType type);
    const QueryMetrics& getMetrics(QueryType type) const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<QueryType, std::unique_ptr<QueryMetrics>> metrics_;
};

#endif