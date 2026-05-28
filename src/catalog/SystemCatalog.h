#ifndef DBMS_PAIN_SYSTEMCATALOG_H
#define DBMS_PAIN_SYSTEMCATALOG_H

#include <string>
#include <vector>

class SystemCatalog {
public:
    explicit SystemCatalog(std::string  dataDir);

    // вызывается при CREATE DATABASE
    void addDatabase(const std::string& name);

    // вызывается при DROP DATABASE
    void removeDatabase(const std::string& name);

    // существует ли БД
    bool hasDatabase(const std::string& name) const;

    // список всех БД (нужен при старте, чтобы загрузить все)
    [[nodiscard]] std::vector<std::string> listDatabases() const;

private:
    std::string dataDir_;
    std::vector<std::string> databases_;

    void load();  // читает catalog.dat при старте
    void save() const;  // перезаписывает catalog.dat после каждого изменения
};

#endif // DBMS_PAIN_SYSTEMCATALOG_H