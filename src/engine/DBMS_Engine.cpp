#include "engine/DBMS_Engine.h"
#include <iostream>
#include "engine/Executor.h"
#include "logger/AccessLogger.h"
#include "logger/LogRecord.h"
#include "metrics/MetricsCollector.h"

namespace dbms {
    DBMSEngine::DBMSEngine(const std::string& logFilePath) :
        logger_(logFilePath), undoLogManager_("./data/undo.log"), executor_(&undoLogManager_),
        reporter_{std::chrono::seconds(10)} {
        reporter_.start([](const std::string& stats) { std::cout << stats; });
    }


    static QueryType detectQueryType(const std::string& query) {
        size_t i = query.find_first_not_of(" \t\n\r");
        if (i == std::string::npos)
            return QueryType::OTHER;

        size_t end = query.find_first_of(" \t\n\r(;", i);
        std::string word = query.substr(i, end - i);
        for (auto& c: word)
            c = std::toupper(c);

        if (word == "SELECT")
            return QueryType::SELECT;
        if (word == "INSERT")
            return QueryType::INSERT;
        if (word == "UPDATE")
            return QueryType::UPDATE;
        if (word == "DELETE")
            return QueryType::DELETE;
        return QueryType::OTHER; // CREATE, DROP, USE, REVERT — сюда
    }


    void DBMSEngine::processQueryBuffer(const std::string& queryText,
                                        const std::string& sessionId) {
        if (queryText.find_first_not_of(" \t\n\r") == std::string::npos)
            return;

        auto startTime = std::chrono::steady_clock::now();

        LogRecord record(queryText, sessionId, "parser");

        QueryType qtype = detectQueryType(queryText);

        try {
            std::unique_ptr<ASTNode> astRoot = parser_.parse(queryText);

            if (!astRoot) {
                std::cerr << "Syntax Error: " << parser_.getLastError() << std::endl;
                record.status_code = LogStatus::SYNTAX_ERROR;
                record.finalize();
                logger_.append(record);

                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - startTime);
                MetricsCollector::instance().recordQuery(qtype, latency, false);
                return;
            }

            record.handler_id = "executor";
            QueryResult result = executor_.execute(astRoot.get());

            bool success = result.ok;
            if (!success) {
                std::cerr << "Execution Error: " << result.error << std::endl;
                record.status_code = LogStatus::EXECUTION_ERROR;
            } else {
                record.status_code = LogStatus::OK;
                if (!result.rows.empty())
                    std::cout << executor_.toJSON(result.rows) << std::endl;
                else
                    std::cout << "Query OK, " << result.affected << " rows affected." << std::endl;
            }

            record.finalize();
            logger_.append(record);

            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - startTime);
            MetricsCollector::instance().recordQuery(qtype, latency, success);

        } catch (const std::exception& e) {
            std::cerr << "Runtime Exception: " << e.what() << std::endl;
            record.status_code = LogStatus::INTERNAL_ERROR;
            record.finalize();
            logger_.append(record);

            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - startTime);
            MetricsCollector::instance().recordQuery(qtype, latency, false);
        }
    }


} // namespace dbms
