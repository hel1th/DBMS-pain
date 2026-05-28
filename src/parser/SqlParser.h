#ifndef DBMS_PAIN_SQLPARSER_H
#define DBMS_PAIN_SQLPARSER_H

#include <memory>
#include <string>

class ASTNode;
class SqlParser {
public:
    SqlParser();
    ~SqlParser();

    // Разобрать запрос, nullptr - ошибка
    std::unique_ptr<ASTNode> parse(const std::string& query);

    // Проверить синтаксис
    bool validate(const std::string& query);

    // Получить последнюю ошибку
    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // DBMS_PAIN_SQLPARSER_H
