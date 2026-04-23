#ifndef DBMS_PAIN_SCHEMA_H
#define DBMS_PAIN_SCHEMA_H

#include <string>
#include <vector>

enum class ColType { INT, STRING };

struct ColumnDef {
    std::string name;
    ColType     type;
    bool        not_null  = false;
    bool        indexed   = false;
    // задание 10
    // std::optional<Value> default_value;
};

struct Schema {
    std::string            table_name;
    std::vector<ColumnDef> columns;

    // Найти индекс колонки по имени (-1 если не найдена)
    int column_index(const std::string& name) const {
        for (int i = 0; i < static_cast<int>(columns.size()); i++)
            if (columns[i].name == name) return i;
        return -1;
    }
};


#endif //DBMS_PAIN_SCHEMA_H