#pragma once

#include <chrono>
#include <string>

namespace dbms {

    enum class LogStatus {
        OK = 0, // Успешное выполнение
        SYNTAX_ERROR = 1, // Ошибка синтаксиса SQL
        EXECUTION_ERROR = 2, // Ошибка выполнения запроса
        AUTH_ERROR = 3, // Ошибка авторизации
        IO_ERROR = 4, // Ошибка ввода-вывода
        INTERNAL_ERROR = 5 // Внутренняя ошибка системы
    };

    struct LogRecord {
        std::string request_body; // Тело SQL-запроса
        std::string session_id; // Идентификатор клиента (session_id)
        std::string handler_id; // Компонент: "parser", "executor", "storage"
        std::chrono::system_clock::time_point timestamp;
        std::chrono::steady_clock::time_point start_time; // Время начала
        std::chrono::steady_clock::time_point end_time; // Время завершения
        LogStatus status_code; // Статус выполнения


        LogRecord() :
            timestamp(std::chrono::system_clock::now()),
            start_time(std::chrono::steady_clock::now()), end_time(start_time) {}


        LogRecord(std::string req, std::string session, std::string handler) :
            request_body(std::move(req)), session_id(std::move(session)),
            handler_id(std::move(handler)), timestamp(std::chrono::system_clock::now()),
            start_time(std::chrono::steady_clock::now()), end_time(start_time) {}


        long long durationMs() const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time)
                    .count();
        }

        void finalize() { end_time = std::chrono::steady_clock::now(); }
    };

} // namespace dbms
