#ifndef DBMS_PAIN_EXECUTOR_H
#define DBMS_PAIN_EXECUTOR_H
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Database.h"
#include "catalog/SystemCatalog.h"
#include "parser/AST.h"
#include "utils/Value.h"


using Row = std::vector<std::pair<std::string, Value>>;


struct QueryResult {
    bool ok = true;
    std::string error;
    std::vector<Row> rows; // SELECTed rows
    int affected = 0; // info about INSERT/UPDATE/DELETE
};
class Executor {
public:
    Executor();

    QueryResult execute(ASTNode* node);

    std::string toJSON(const std::vector<Row>& rows);

private:
    std::string currentDb_;
    std::unordered_map<std::string, std::unique_ptr<Database>> databases_;
    SystemCatalog catalog_;

    QueryResult execInsert(const InsertQuery& q);
    QueryResult execSelect(const SelectQuery& q);
    QueryResult execUpdate(const UpdateQuery& q);
    QueryResult execDelete(const DeleteQuery& q);
    QueryResult execCreateTable(const CreateTableQuery& q);
    QueryResult execDropTable(const DropTableQuery& q);
    QueryResult execCreateDatabase(const CreateDatabaseQuery& q);
    QueryResult execDropDatabase(const DropDatabaseQuery& q);
    QueryResult execUse(const UseQuery& q);

    Database& currentDatabase();

    // Использует resolve, чтобы вычислить обе стороны условия WHERE, обходя дерево рекурсивно
    bool matches(const std::vector<Value>& record, const Schema& schema, const ASTNode* where);

    Value resolve(const ASTNode* node, const std::vector<Value>& record, const Schema& schema);

    static Row project(const std::vector<Value>& record, const Schema& schema,
                       const SelectQuery& q);
};


#endif // DBMS_PAIN_EXECUTOR_H
