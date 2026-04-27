#ifndef DBMS_PAIN_SCHEMA_H
#define DBMS_PAIN_SCHEMA_H

#include <string>
#include <vector>

enum class ColType { INT, STRING };

struct ColumnDef {
    std::string name;
    ColType     type;
    bool        notNull  = false;
    bool        indexed   = false;
    // задание 10
    // std::optional<Value> default_value;
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
    
    size_t GetColumnCount() const { return columns.size(); }
    ColType GetColumnType(size_t index) const { return columns[index].type; }
    const std::string& GetColumnName(size_t index) const { return columns[index].name; }
};


#endif //DBMS_PAIN_SCHEMA_H