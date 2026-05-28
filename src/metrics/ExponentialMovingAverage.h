#ifndef DBMS_PAIN_EXPONENTIAL_MOVING_AVERAGE_H
#define DBMS_PAIN_EXPONENTIAL_MOVING_AVERAGE_H

#include <mutex>

class ExponentialMovingAverage {
public:
    explicit ExponentialMovingAverage(double alpha = 0.1);
    
    void add(double value);
    double get() const;
    void reset();
    void setAlpha(double alpha);
    
private:
    mutable std::mutex mutex_;
    double alpha_;
    bool initialized_;
    double value_;
};

#endif