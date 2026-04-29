#ifndef DBMS_PAIN_SCHEMA_H
#define DBMS_PAIN_SCHEMA_H

#include <string>
#include <vector>

#include "utils/Value.h"

enum class ColType { INT, STRING };

struct ColumnDef {
    std::string name;
    ColType     type;
    bool        notNull  = false;
    bool        indexed   = false;
    std::optional<Value> default_value;
};

struct Schema {
    std::string            tableName;
    std::vector<ColumnDef> columns;

    // Найти индекс колонки по имени (-1 если не найдена)
    int columnIndex(const std::string& name) const {
        for (int i = 0; i < static_cast<int>(columns.size()); i++)
            if (columns[i].name == name) return i;
        return -1;
    }
    int indexedColumn() const {
        for (int i = 0; i < static_cast<int>(columns.size()); ++i)
            if (columns[i].indexed) return i;

        return -1;
    }
};


#endif //DBMS_PAIN_SCHEMA_H