#include "SystemCatalog.h"
#include "utils/Error.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <utility>

const std::string CatalogFile = "/catalog.dat";

SystemCatalog::SystemCatalog(std::string  dataDir) : dataDir_(std::move(dataDir)) {
    std::filesystem::create_directories(dataDir_);
    load();
}

void SystemCatalog::load() {
    std::ifstream f(dataDir_ + CatalogFile);
    if (!f.is_open()) return;

    std::string name;

    while (std::getline(f, name)) {
        if (!name.empty())
            databases_.push_back(name);
    }
}

void SystemCatalog::save() const {
    std::ofstream f(dataDir_+ CatalogFile, std::ios::trunc);

    if (!f.is_open())
        throw StorageError("Cannot write " + CatalogFile);

    for (auto& name : databases_)
        f << name << "\n";
}

void SystemCatalog::addDatabase(const std::string& name) {
    if (hasDatabase(name))
        throw SemanticError("Database already exists: " + name);

    databases_.push_back(name);
    std::filesystem::create_directories(dataDir_+ "/" + name);

    save();
}

void SystemCatalog::removeDatabase(const std::string& name) {
    auto it = std::ranges::find(databases_, name);
    if (it == databases_.end())
        throw SemanticError("Database does not exist: " + name);

    databases_.erase(it);

    std::filesystem::remove_all(dataDir_ + "/" + name);

    save();
}

bool SystemCatalog::hasDatabase(const std::string& name) const {
    return std::ranges::find(databases_, name) != databases_.end();
}

std::vector<std::string> SystemCatalog::listDatabases() const {
    return databases_;
}