#include "Database.h"
#include "utils/Error.h"
#include <fstream>
#include <sstream>
#include <filesystem>

Database::Database(const std::string& db_path, const std::string& name)
    : name_(name), dbPath_(db_path)
{
    loadTables();
}

void Database::createTable(const Schema& schema) {
    if (hasTable(schema.tableName))
        throw SemanticError("Table already exists: " + schema.tableName);

    tables_[schema.tableName] = Table::create(dbPath_, schema);
    saveSchema();
}

void Database::dropTable(const std::string& name) {
    if (!hasTable(name))
        throw SemanticError("Table does not exist: " + name);

    tables_.erase(name);
    Table::drop(dbPath_, name);
    saveSchema();
}

Table& Database::getTable(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end())
        throw SemanticError("Table does not exist: " + name);
    return *it->second;
}

bool Database::hasTable(const std::string& name) const {
    return tables_.find(name) != tables_.end();
}

void Database::loadTables() {
    std::ifstream f(dbPath_ + "/schema.dat");
    if (!f.is_open()) return; // новая БД — таблиц ещё нет

    std::string line;
    Schema current;
    bool inTable = false;

    while (std::getline(f, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "TABLE") {
            if (inTable && !current.tableName.empty())
                tables_[current.tableName] = std::make_unique<Table>(
                    dbPath_, current.tableName
                );
            current = Schema{};
            ss >> current.tableName;
            inTable = true;
        } else {
            // колонка: name TYPE [INDEXED] [NOT_NULL] [DEFAULT value]
            ColumnDef col;
            col.name = token;

            std::string typeStr;
            ss >> typeStr;
            col.type = (typeStr == "INT") ? ColType::INT : ColType::STRING;

            std::string mod;
            while (ss >> mod) {
                if (mod == "INDEXED")  col.indexed = true;
                else if (mod == "NOT_NULL") col.notNull = true;
                else if (mod == "DEFAULT") {
                    std::string val;
                    ss >> val;
                    if (col.type == ColType::INT) {
                        col.default_value = Value(std::stoi(val));
                    } else {
                        // убираем кавычки если есть
                        if (!val.empty() && val.front() == '"')
                            val = val.substr(1, val.size() - 2);
                        col.default_value = Value(val);
                    }
                }
            }
            current.columns.push_back(col);
        }
    }

    // последняя таблица
    if (inTable && !current.tableName.empty())
        tables_[current.tableName] = std::make_unique<Table>(
            dbPath_, current.tableName
        );
}

void Database::saveSchema() {
    std::ofstream f(dbPath_ + "/schema.dat", std::ios::trunc);
    if (!f.is_open())
        throw StorageError("Cannot write schema.dat for: " + name_);

    for (const auto& [name, tbl] : tables_) {
        f << "TABLE " << name << "\n";
        for (const auto& col : tbl->schema().columns) {
            f << col.name << " ";
            f << (col.type == ColType::INT ? "INT" : "STRING");
            if (col.indexed)  f << " INDEXED";
            if (col.notNull)  f << " NOT_NULL";
            if (col.default_value.has_value()) {
                f << " DEFAULT ";
                if (val::isInt(*col.default_value))
                    f << val::getInt(*col.default_value);
                else
                    f << "\"" << val::getString(*col.default_value) << "\"";
            }
            f << "\n";
        }
    }
}