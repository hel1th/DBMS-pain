#ifndef DBMS_PAIN_TABLE_H
#define DBMS_PAIN_TABLE_H

#include <functional>
#include <string>
#include "Schema.h"
#include "index/IndexManager.h"
#include "storage/PageManager.h"
#include "storage/RecordManager.h"


class Table {
public:
    // Открыть существующую таблицу
    Table(const std::string& dbPath, const std::string& tableName);

    // Создать новую таблицу
    static Table create(const std::string& dbPath, const Schema& schema);

    // Удалить таблицу (файлы с диска)
    static void drop(const std::string& dbPath, const std::string& tableName);

    const Schema& schema() const { return schema_; }

    // DML (data manipulation language)
    RecordID insert(const std::vector<Value>& record);

    void scan(std::function<void(RecordID, const std::vector<Value>&)> cb);

    // Поиск по индексу — вернёт RecordID или бросит если нет индекса
    RecordID findByIndex(const std::string& colName, const Value& key);

    void update(RecordID rID, const std::vector<Value>& newRecord);
    void remove(RecordID rID);

private:
    Schema schema_;
    PageManager pageManager_;
    RecordManager recordManager_;
    IndexManager indexManager_;

    std::string dbPath_;
};


#endif // DBMS_PAIN_TABLE_H
