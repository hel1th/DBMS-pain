#ifndef DBMS_PAIN_TABLE_H
#define DBMS_PAIN_TABLE_H

#include <functional>
#include <memory>
#include <string>
#include "Schema.h"
#include "index/include/IndexManager.h"
#include "storage/PageManager.h"
#include "storage/RecordManager.h"


class Table {
public:
    Table() = default;
    Table(const std::string& dbPath, const std::string& tableName);

    // Создать новую таблицу
    static std::unique_ptr<Table> create(const std::string& dbPath, const Schema& schema);

    // Удалить таблицу (файлы с диска)
    static void drop(const std::string& dbPath, const std::string& tableName);

    [[nodiscard]] const Schema& schema() const { return schema_; }

    // DML (data manipulation language)
    RecordID insert(const std::vector<Value>& record);

    void scan(std::function<void(RecordID, const std::vector<Value>&)> cb) const;

    // Поиск по индексу — вернёт RecordID или бросит если нет индекса
    RecordID findByIndex(const std::string& colName, const Value& key);
    void update(RecordID rid, const std::vector<Value>& newRecord);
    void remove(RecordID rid);
    std::vector<Value> fetch(RecordID rid);

private:
    Schema schema_;
    std::string dbPath_;
    std::unique_ptr<PageManager> pageManager_;
    std::unique_ptr<RecordManager> recordManager_;
    std::unique_ptr<IndexManager> indexManager_; // nullptr если нет INDEXED колонки
};


#endif // DBMS_PAIN_TABLE_H
