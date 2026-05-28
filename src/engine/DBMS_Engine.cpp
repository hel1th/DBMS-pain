#include "engine/DBMS_Engine.h"
#include <iostream>
#include "logger/AccessLogger.h"
#include "logger/LogRecord.h"

namespace dbms {

    DBMSEngine::DBMSEngine(const std::string& logFilePath) : logger_(logFilePath) {}

    void DBMSEngine::processQueryBuffer(const std::string& queryText,
                                        const std::string& sessionId) {
        // Пропускаем пустые запросы (пробелы, переносы строк)
        if (queryText.find_first_not_of(" \t\n\r") == std::string::npos) {
            return;
        }

        // Конструктор фиксирует текущее системное время (timestamp) и старт
        LogRecord record(queryText, sessionId, "parser");

        try {
            std::unique_ptr<ASTNode> astRoot = parser_.parse(queryText);

            if (!astRoot) {
                std::cerr << "Syntax Error: " << parser_.getLastError() << std::endl;

                record.status_code = LogStatus::SYNTAX_ERROR;
                record.finalize();
                logger_.append(record);
                return;
            }

            record.handler_id = "executor";
            QueryResult result = executor_.execute(astRoot.get());

            if (!result.ok) {
                std::cerr << "Execution Error: " << result.error << std::endl;
                record.status_code = LogStatus::EXECUTION_ERROR;
            } else {
                record.status_code = LogStatus::OK;

                if (!result.rows.empty()) {
                    std::cout << executor_.toJSON(result.rows) << std::endl;
                } else {
                    std::cout << "Query OK, " << result.affected << " rows affected." << std::endl;
                }
            }

            // duration и сохраняем
            record.finalize();
            logger_.append(record);

        } catch (const std::exception& e) {
            std::cerr << "Runtime Exception: " << e.what() << std::endl;

            record.status_code = LogStatus::INTERNAL_ERROR;
            record.finalize();
            logger_.append(record);
        }
    }

} // namespace dbms
