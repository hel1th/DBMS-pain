#include "metrics/MetricsReporter.h"
#include <iostream>
#include "metrics/MetricsCollector.h"

MetricsReporter::~MetricsReporter() { stop(); }

void MetricsReporter::start(Callback callback) {
    if (running_)
        return;

    callback_ = callback;
    running_ = true;
    reporterThread_ = std::thread([this]() { run(); });
}

void MetricsReporter::stop() {
    running_ = false;
    if (reporterThread_.joinable()) {
        reporterThread_.join();
    }
}

void MetricsReporter::report() const {
    std::string stats = MetricsCollector::instance().getStats();

    if (callback_) {
        callback_(stats);
    } else {
        std::cout << stats;
    }
}

void MetricsReporter::run() {
    while (running_) {
        std::this_thread::sleep_for(intervalMs_);
        if (running_) {
            report();
        }
    }
}
