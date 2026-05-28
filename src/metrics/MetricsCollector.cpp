#include "metrics/MetricsCollector.h"
#include <iomanip>
#include <sstream>

MetricsCollector& MetricsCollector::instance() {
    static MetricsCollector collector;
    return collector;
}

void MetricsCollector::recordQuery(QueryType type, std::chrono::microseconds latency,
                                   bool success) {
    auto& metrics = getMetrics(type);

    metrics.rps.add();
    metrics.rps10min.add();
    metrics.avgLatency.add(static_cast<double>(latency.count()) / 1000.0);

    if (!success) {
        metrics.errorRate.add();
    }
}

double MetricsCollector::getCurrentRps(QueryType type) const {
    const auto& metrics = getMetrics(type);
    return static_cast<double>(metrics.rps.count());
}

double MetricsCollector::getAvgRps(QueryType type) const {
    const auto& metrics = getMetrics(type);
    return static_cast<double>(metrics.rps10min.count()) / 600.0;
}

size_t MetricsCollector::getMaxRps(QueryType type) const {
    const auto& metrics = getMetrics(type);
    return metrics.rps10min.max();
}

double MetricsCollector::getAvgLatencyMs(QueryType type) const {
    const auto& metrics = getMetrics(type);
    return metrics.avgLatency.get();
}

size_t MetricsCollector::getErrorCount(QueryType type) const {
    const auto& metrics = getMetrics(type);
    return metrics.errorRate.count();
}

double MetricsCollector::getErrorRate(QueryType type) const {
    const auto& metrics = getMetrics(type);
    size_t total = metrics.rps10min.count();
    if (total == 0)
        return 0.0;
    return static_cast<double>(metrics.errorRate.count()) / total * 100.0;
}

std::string MetricsCollector::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    oss << "\n========== TELEMETRY STATS ==========\n";

    for (const auto& pair: metrics_) {
        QueryType type = pair.first;
        const auto& metrics = *pair.second;

        oss << "\n[" << queryTypeToString(type) << "]\n";
        oss << std::fixed << std::setprecision(2);
        oss << "  Current RPS:   " << static_cast<double>(metrics.rps.count()) << "\n";
        oss << "  Avg RPS (10m): " << static_cast<double>(metrics.rps10min.count()) / 600.0 << "\n";
        oss << "  Max RPS (10m): " << metrics.rps10min.max() << "\n";
        oss << "  Avg latency:   " << metrics.avgLatency.get() << " ms\n";
        oss << "  Error count:   " << metrics.errorRate.count() << "\n";
        oss << "  Error rate:    "
            << (metrics.rps10min.count() == 0 ? 0.0
                                              : static_cast<double>(metrics.errorRate.count()) /
                                                        metrics.rps10min.count() * 100.0)
            << "%\n";
    }

    oss << "=======================================\n";
    return oss.str();
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair: metrics_) {
        auto& metrics = *pair.second;
        metrics.rps.reset();
        metrics.rps10min.reset();
        metrics.errorRate.reset();
        metrics.avgLatency.reset();
    }
}

MetricsCollector::QueryMetrics& MetricsCollector::getMetrics(QueryType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(type);
    if (it == metrics_.end()) {
        auto ptr = std::make_unique<QueryMetrics>();
        it = metrics_.emplace(type, std::move(ptr)).first;
    }
    return *it->second;
}

const MetricsCollector::QueryMetrics& MetricsCollector::getMetrics(QueryType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    static QueryMetrics emptyMetrics;
    auto it = metrics_.find(type);
    if (it == metrics_.end()) {
        return emptyMetrics;
    }
    return *it->second;
}
