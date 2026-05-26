#include "../include/ExponentialMovingAverage.h"

ExponentialMovingAverage::ExponentialMovingAverage(double alpha)
    : alpha_(alpha), initialized_(false), value_(0.0) {}

void ExponentialMovingAverage::add(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        value_ = value;
        initialized_ = true;
    } else {
        value_ = alpha_ * value + (1.0 - alpha_) * value_;
    }
}

double ExponentialMovingAverage::get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

void ExponentialMovingAverage::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
    value_ = 0.0;
}

void ExponentialMovingAverage::setAlpha(double alpha) {
    std::lock_guard<std::mutex> lock(mutex_);
    alpha_ = alpha;
}