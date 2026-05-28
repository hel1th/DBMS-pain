#ifndef DBMS_PAIN_ROLLING_COUNTER_H
#define DBMS_PAIN_ROLLING_COUNTER_H

#include <algorithm>
#include <chrono>
#include <mutex>
#include <vector>

class RollingCounter {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    template<typename Rep, typename Period>
    explicit RollingCounter(const std::chrono::duration<Rep, Period>& windowSize) :
        windowSize_(std::chrono::duration_cast<std::chrono::seconds>(windowSize)), maxCount_(0) {
        if (windowSize_.count() <= 0) {
            windowSize_ = std::chrono::seconds(1);
        }
    }

    void add();
    size_t count() const;
    size_t max() const;
    void updateMax();
    void reset();

private:
    void cleanup(const TimePoint& now) const;
    void updateMaxInternal(const TimePoint& now);

private:
    mutable std::mutex mutex_;
    std::chrono::seconds windowSize_;
    mutable std::vector<TimePoint> timestamps_;
    mutable size_t maxCount_;
};

#endif
