#ifndef DBMS_PAIN_TABLE_H
#define DBMS_PAIN_TABLE_H

#include <functional>
#include <string>
#include "Schema.h"
#include "index/IndexManager.h"
#include "parser/AST.h"
#include "storage/PageManager.h"
#include "storage/RecordManager.h"


class Table {
public:
    // Открыть существующую таблицу
    Table(const std::string& db_path, const std::string& table_name);

    // Создать новую таблицу
    static Table create(const std::string& db_path, const Schema& schema);

    // Удалить таблицу (файлы с диска)
    static void drop(const std::string& db_path, const std::string& table_name);

    const Schema& schema() const { return schema_; }

    // DML (data manipulation language)
    RecordId insert(const std::vector<Value>& record);

    void scan(std::function<void(RecordId, const std::vector<Value>&)> cb);

    // Поиск по индексу — вернёт RecordId или бросит если нет индекса
    RecordId find_by_index(const std::string& col_name, const Value& key);

    void update(RecordId rid, const std::vector<Value>& new_record);
    void remove(RecordId rid);

private:
    Schema schema_;
    PageManager page_manager_;
    RecordManager record_manager_;
    IndexManager index_manager_;

    std::string db_path_;
};


#endif // DBMS_PAIN_TABLE_H
