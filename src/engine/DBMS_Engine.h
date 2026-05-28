#pragma once

#include <string>
#include "engine/Executor.h"
#include "logger/AccessLogger.h"
#include "metrics/MetricsReporter.h"
#include "parser/SqlParser.h"
#include "undo-log/UndoLogManager.h"

namespace dbms {

    class DBMSEngine {
    public:
        explicit DBMSEngine(const std::string& logFilePath = "data/access.log");
        ~DBMSEngine() = default;

        DBMSEngine(const DBMSEngine&) = delete;
        DBMSEngine& operator=(const DBMSEngine&) = delete;

        void processQueryBuffer(const std::string& queryText,
                                const std::string& sessionId = "default");

    private:
        AccessLogger logger_;
        UndoLogManager undoLogManager_;
        Executor executor_;
        SqlParser parser_;
        MetricsReporter reporter_{std::chrono::seconds(10)};
    };

} // namespace dbms
