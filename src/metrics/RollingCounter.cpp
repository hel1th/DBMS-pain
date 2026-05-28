#include "metrics/RollingCounter.h"

void RollingCounter::add() {
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    timestamps_.push_back(now);
    cleanup(now);
}

size_t RollingCounter::count() const {
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    cleanup(now);
    return timestamps_.size();
}

size_t RollingCounter::max() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return maxCount_;
}

void RollingCounter::updateMax() {
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    updateMaxInternal(now);
}

void RollingCounter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    timestamps_.clear();
    maxCount_ = 0;
}

void RollingCounter::cleanup(const TimePoint& now) const {
    auto cutoff = now - windowSize_;
    timestamps_.erase(std::remove_if(timestamps_.begin(), timestamps_.end(),
                                     [cutoff](const TimePoint& tp) { return tp <= cutoff; }),
                      timestamps_.end());
}

void RollingCounter::updateMaxInternal(const TimePoint& now) {
    cleanup(now);
    if (timestamps_.size() > maxCount_) {
        maxCount_ = timestamps_.size();
    }
}
