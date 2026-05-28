#ifndef DBMS_PAIN_DATABASE_H
#define DBMS_PAIN_DATABASE_H

#include <memory>
#include <string>
#include <unordered_map>
#include "Table.h"


class Database {
public:
    explicit Database(const std::string& db_path, const std::string& name);
    // DDL (data definition language)
    void createTable(const Schema& schema);
    void dropTable(const std::string& name);

    // Получить таблицу (бросает SemanticError если нет)
    Table& getTable(const std::string& name);

    bool hasTable(const std::string& name) const;

    std::string name() const { return name_; }

private:
    std::string name_;
    std::string dbPath_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;

    void loadTables(); // читает список таблиц из каталога при старте
    void saveSchema();
};


#endif // DBMS_PAIN_DATABASE_H
