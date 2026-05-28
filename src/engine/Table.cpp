#include "Table.h"
#include <filesystem>
#include <utility>
#include "utils/Error.h"

// Путь к файлу данных таблицы
static std::string dataPath(const std::string& dbPath, const std::string& tableName) {
    return dbPath + "/" + tableName + ".dat";
}

// Путь к файлу индекса
static std::string indexPath(const std::string& dbPath, const std::string& tableName,
                             const std::string& colName) {
    return dbPath + "/" + tableName + "_" + colName + ".idx";
}


Table::Table(const std::string& dbPath, const Schema& schema) :
    schema_(schema), dbPath_(dbPath) // Сразу копируем схему целиком
{
    pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema_.tableName));
    recordManager_ = std::make_unique<RecordManager>(*pageManager_, schema_);

    // Индекс — только если есть INDEXED колонка
    int idxCol = schema_.indexedColumn();
    if (idxCol != -1) {
        indexManager_ = std::make_unique<IndexManager>(
                indexPath(dbPath, schema_.tableName, schema_.columns[idxCol].name));
    }
}

// Статический метод — создать новую таблицу
std::unique_ptr<Table> Table::create(const std::string& dbPath, const Schema& schema) {
    auto tbl = std::make_unique<Table>();
    tbl->schema_ = schema;
    tbl->dbPath_ = dbPath;
    tbl->pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema.tableName));
    tbl->recordManager_ = std::make_unique<RecordManager>(*tbl->pageManager_, tbl->schema_);

    int idxCol = schema.indexedColumn();
    if (idxCol != -1) {
        tbl->indexManager_ = std::make_unique<IndexManager>(
                indexPath(dbPath, schema.tableName, schema.columns[idxCol].name));
    }

    return tbl;
}

// Удалить файлы таблицы с диска
void Table::drop(const std::string& dbPath, const std::string& tableName) {
    std::filesystem::remove(dataPath(dbPath, tableName));
    // удаляем все .idx файлы этой таблицы
    for (auto& entry: std::filesystem::directory_iterator(dbPath)) {
        auto name = entry.path().filename().string();
        if (name.rfind(tableName + "_", 0) == 0 && entry.path().extension() == ".idx")
            std::filesystem::remove(entry.path());
    }
}

RecordID Table::insert(const std::vector<Value>& record) {
    RecordID rid = recordManager_->insert(record);

    // обновить индекс если есть
    if (indexManager_) {
        int idxCol = schema_.indexedColumn();
        const Value& key = record[idxCol];
        if (val::isNull(key))
            throw SemanticError("INDEXED column cannot be NULL");
        indexManager_->insertKey(key, rid);
    }

    return rid;
}

void Table::scan(std::function<void(RecordID, const std::vector<Value>&)> cb) const {
    recordManager_->scan(std::move(cb));
}

RecordID Table::findByIndex(const std::string& colName, const Value& key) {
    if (!indexManager_)
        throw IndexError("No index on column: " + colName);
    return indexManager_->findKey(key);
}

void Table::update(RecordID rid, const std::vector<Value>& newRecord) {
    // обновить индекс: удалить старый ключ, вставить новый
    if (indexManager_) {
        int idxCol = schema_.indexedColumn();
        auto oldRecord = recordManager_->fetch(rid);
        indexManager_->removeKey(oldRecord[idxCol]);
        indexManager_->insertKey(newRecord[idxCol], rid);
    }

    recordManager_->update(rid, newRecord);
}

void Table::remove(RecordID rid) {
    if (indexManager_) {
        int idxCol = schema_.indexedColumn();
        auto record = recordManager_->fetch(rid);
        indexManager_->removeKey(record[idxCol]);
    }

    recordManager_->remove(rid);
}

std::vector<Value> Table::fetch(RecordID rid) { return recordManager_->fetch(rid); }
