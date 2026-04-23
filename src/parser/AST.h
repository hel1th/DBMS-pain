#include <memory>
#include <string>
#include <vector>

#include "engine/Schema.h" // ColType
#include "utils/Value.h"


// Виды узлов
enum class NodeKind {
    SELECT_QUERY,
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_QUERY,
    CREATE_TABLE_QUERY,
    DROP_TABLE_QUERY,
    CREATE_DATABASE_QUERY,
    DROP_DATABASE_QUERY,
    USE_QUERY,
    LITERAL,
    COLUMN_REF,
    BINARY_OP,
    AND_OP,
    OR_OP,
    BETWEEN_OP,
    LIKE_OP,
    STAR
};

// Базовый класс узла
class ASTNode {
public:
    NodeKind kind;
    explicit ASTNode(NodeKind k);
    virtual ~ASTNode() = default;
    [[nodiscard]] virtual std::string toString() const = 0;
};

// Листовые узлы
class Star : public ASTNode {
public:
    Star();
    [[nodiscard]] std::string toString() const override;
};

class ColumnRef : public ASTNode {
public:
    std::string name;
    explicit ColumnRef(const std::string& n);
    [[nodiscard]] std::string toString() const override;
};

class Literal : public ASTNode {
public:
    Value value;
    explicit Literal(Value v);
    [[nodiscard]] std::string toString() const override;

    // используем функции из Value.h — не дублируем
    bool isNull() const { return val::isNull(value); }
    bool isInt() const { return val::isInt(value); }
    bool isString() const { return val::isString(value); }
    int asInt() const { return val::getInt(value); }
    const std::string& asString() const { return std::get<std::string>(*value); }
};

// Операторы условий
class BinaryOp : public ASTNode {
public:
    std::string op; // "==", "!=", "<", ">", "<=", ">="
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOp(const std::string& o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class AndOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    AndOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class OrOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    OrOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class BetweenOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> low;
    std::unique_ptr<ASTNode> high;
    BetweenOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> h);
    [[nodiscard]] std::string toString() const override;
};

class LikeOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> pattern;
    LikeOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> p);
    [[nodiscard]] std::string toString() const override;
};

// Команды

// SELECT col AS alias, col2, * FROM table WHERE ...
struct SelectColumn {
    std::string name;
    std::string alias; // пустая строка = нет алиаса
};

struct AggregateExpr {
    std::string func; // "SUM", "COUNT", "AVG"
    std::string column; // имя колонки или "*" для COUNT(*)
};

class SelectQuery : public ASTNode {
public:
    bool star = false; // SELECT *
    std::vector<SelectColumn> columns; // SELECT col, col AS alias
    std::vector<AggregateExpr> aggregates; // SELECT SUM(col)
    std::string tableName;
    std::unique_ptr<ASTNode> where; // nullptr = нет WHERE

    SelectQuery();
    [[nodiscard]] std::string toString() const override;
};

class InsertQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::string> columns; // перечисленные колонки
    // каждый вложенный вектор = одна строка VALUES
    std::vector<std::vector<std::unique_ptr<ASTNode>>> values;

    InsertQuery();
    [[nodiscard]] std::string toString() const override;
};

class UpdateQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::pair<std::string,
                          std::unique_ptr<ASTNode>>> assignments; // col = expr
    std::unique_ptr<ASTNode> where;

    UpdateQuery();
    [[nodiscard]] std::string toString() const override;
};

class DeleteQuery : public ASTNode {
public:
    std::string tableName;
    std::unique_ptr<ASTNode> where;

    DeleteQuery();
    [[nodiscard]] std::string toString() const override;
};

// определение колонки при CREATE TABLE
struct ColumnSpec {
    std::string name;
    ColType type; // ColType::INT или ColType::STRING
    bool notNull = false;
    bool indexed = false;
    Value defaultValue; // nullopt = нет DEFAULT
};

class CreateTableQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<ColumnSpec> columns;

    CreateTableQuery();
    [[nodiscard]] std::string toString() const override;
};

class DropTableQuery : public ASTNode {
public:
    std::string tableName;
    explicit DropTableQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class CreateDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit CreateDatabaseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class DropDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit DropDatabaseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class UseQuery : public ASTNode {
public:
    std::string dbName;
    explicit UseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};
