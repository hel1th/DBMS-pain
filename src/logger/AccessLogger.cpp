#include "AccessLogger.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace dbms {


    AccessLogger::AccessLogger(const std::string& filepath) {
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        file_.open(filepath, std::ios::app);
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open log file: " + filepath);
        }
        file_.seekp(0, std::ios::end);
        if (file_.tellp() == 0) {
            file_ << "# timestamp | session_id | handler_id | request | duration_ms | status\n";
        }
    }
    void AccessLogger::append(const LogRecord& rec) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto duration = rec.durationMs();
        auto timestamp = formatTimestamp(rec.timestamp);

        file_ << timestamp << " | " << rec.session_id << " | " << rec.handler_id << " | "
              << rec.request_body << " | " << duration << "ms" << " | "
              << statusToString(rec.status_code) << "\n";

        file_.flush();
    }

    std::string AccessLogger::formatTimestamp(std::chrono::system_clock::time_point tp) const {
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        std::tm tm_buf;
#if defined(_WIN32) || defined(_MSC_VER)
        localtime_s(&tm_buf, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_buf);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    std::string AccessLogger::statusToString(LogStatus status) const {
        switch (status) {
            case LogStatus::OK:
                return "OK";
            case LogStatus::SYNTAX_ERROR:
                return "SYNTAX_ERROR";
            case LogStatus::EXECUTION_ERROR:
                return "EXECUTION_ERROR";
            case LogStatus::AUTH_ERROR:
                return "AUTH_ERROR";
            case LogStatus::IO_ERROR:
                return "IO_ERROR";
            case LogStatus::INTERNAL_ERROR:
                return "INTERNAL_ERROR";
            default:
                return "UNKNOWN";
        }
    }

} // namespace dbms
