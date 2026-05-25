#include "Database.h"

#include <utility>
Database::Database(std::string  db_path, std::string name)
    : name_(std::move(name)), dbPath_(std::move(db_path))
{
    loadTables();
}