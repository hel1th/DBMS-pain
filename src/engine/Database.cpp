#include "Database.h"
Database::Database(const std::string& db_path, const std::string& name)
    : name_(name), dbPath_(db_path)
{
    loadTables();
}