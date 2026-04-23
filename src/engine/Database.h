#ifndef DBMS_PAIN_DATABASE_H
#define DBMS_PAIN_DATABASE_H

#include <memory>
#include <string>
#include <unordered_map>
#include "Table.h"
#include "utils/Error.h"


class Database {
public:
    explicit Database(const std::string& db_path);

    // DDL (data definition language)
    void createTable(const Schema& schema);
    void dropTable(const std::string& name);

    // Получить таблицу (бросает SemanticError если нет)
    Table& getTable(const std::string& name);

    bool hasTable(const std::string& name) const;

    std::string name() const { return name_; }

private:
    std::string name_;
    std::string db_path_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;

    void load_tables(); // читает список таблиц из каталога при старте
};


#endif // DBMS_PAIN_DATABASE_H
