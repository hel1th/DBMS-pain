#ifndef DBMS_PAIN_METRICS_REPORTER_H
#define DBMS_PAIN_METRICS_REPORTER_H

#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

class MetricsReporter {
public:
    using Callback = std::function<void(const std::string&)>;
    
    explicit MetricsReporter(std::chrono::milliseconds interval)
        : intervalMs_(interval)
        , running_(false) {
        if (intervalMs_.count() <= 0) {
            intervalMs_ = std::chrono::milliseconds(100);
        }
    }
    
    explicit MetricsReporter(std::chrono::seconds interval)
        : intervalMs_(interval)
        , running_(false) {
        if (intervalMs_.count() <= 0) {
            intervalMs_ = std::chrono::milliseconds(100);
        }
    }
    
    template<typename Rep, typename Period>
    explicit MetricsReporter(const std::chrono::duration<Rep, Period>& interval)
        : intervalMs_(std::chrono::duration_cast<std::chrono::milliseconds>(interval))
        , running_(false) {
        if (intervalMs_.count() <= 0) {
            intervalMs_ = std::chrono::milliseconds(100);
        }
    }
    
    ~MetricsReporter();
    
    void start(Callback callback = nullptr);
    void stop();
    void report() const;
    
private:
    void run();
    
private:
    std::chrono::milliseconds intervalMs_;
    std::atomic<bool> running_;
    std::thread reporterThread_;
    Callback callback_;
};

#endif // DBMS_PAIN_METRICS_REPORTER_H