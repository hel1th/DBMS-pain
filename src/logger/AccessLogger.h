#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#include "LogRecord.h"

namespace dbms {

    class AccessLogger {
    public:
        explicit AccessLogger(const std::string& filepath);
        ~AccessLogger() = default;

        AccessLogger(const AccessLogger&) = delete;
        AccessLogger& operator=(const AccessLogger&) = delete;
        AccessLogger(AccessLogger&&) = delete;
        AccessLogger& operator=(AccessLogger&&) = delete;

        void append(const LogRecord& rec);

    private:
        std::string formatTimestamp(std::chrono::system_clock::time_point tp) const;
        std::string statusToString(LogStatus status) const;

        std::ofstream file_;
        mutable std::mutex mutex_;
    };

} // namespace dbms
