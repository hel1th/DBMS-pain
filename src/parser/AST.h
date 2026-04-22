#ifndef DBMS_PAIN_AST_H
#define DBMS_PAIN_AST_H

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "../utils/Value.h"

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
    STAR,
    COLUMN_LIST,
    VALUE_LIST,
    ASSIGNMENT_LIST
};

class ASTNode {
public:
    NodeKind kind;
    explicit ASTNode(NodeKind k);
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

class Star : public ASTNode {
public:
    Star();
    std::string toString() const override;
};

class ColumnRef : public ASTNode {
public:
    std::string name;
    explicit ColumnRef(const std::string& n);
    std::string toString() const override;
};

class Literal : public ASTNode {
public:
    Value value;
    explicit Literal(Value v);
    std::string toString() const override;
    bool isNull() const;
    bool isInt() const;
    bool isString() const;
    int asInt() const;
    const std::string& asString() const;
};

class BinaryOp : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOp(const std::string& o,
             std::unique_ptr<ASTNode> l,
             std::unique_ptr<ASTNode> r);
    std::string toString() const override;
};

class AndOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    AndOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    std::string toString() const override;
};

class OrOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    OrOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    std::string toString() const override;
};

class BetweenOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> low;
    std::unique_ptr<ASTNode> high;
    BetweenOp(std::unique_ptr<ASTNode> e,
              std::unique_ptr<ASTNode> l,
              std::unique_ptr<ASTNode> h);
    std::string toString() const override;
};

class LikeOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> pattern;
    LikeOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> p);
    std::string toString() const override;
};

class SelectQuery : public ASTNode {
public:
    std::vector<std::string> columns;
    std::string tableName;
    std::unique_ptr<ASTNode> where;
    std::string alias;
    struct Aggregate {
        std::string func;
        std::string column;
    };
    std::vector<Aggregate> aggregates;
    SelectQuery();
    std::string toString() const override;
};

class InsertQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::string> columns;
    std::vector<std::vector<std::unique_ptr<ASTNode>>> values;
    InsertQuery();
    std::string toString() const override;
};

class UpdateQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> assignments;
    std::unique_ptr<ASTNode> where;
    UpdateQuery();
    std::string toString() const override;
};

class DeleteQuery : public ASTNode {
public:
    std::string tableName;
    std::unique_ptr<ASTNode> where;
    DeleteQuery();
    std::string toString() const override;
};

class CreateTableQuery : public ASTNode {
public:
    std::string tableName;
    struct ColumnDef {
        std::string name;
        std::string type;
        bool notNull;
        bool indexed;
        Value defaultValue;
    };
    std::vector<ColumnDef> columns;
    CreateTableQuery();
    std::string toString() const override;
};

class DropTableQuery : public ASTNode {
public:
    std::string tableName;
    explicit DropTableQuery(const std::string& name);
    std::string toString() const override;
};

class CreateDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit CreateDatabaseQuery(const std::string& name);
    std::string toString() const override;
};

class DropDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit DropDatabaseQuery(const std::string& name);
    std::string toString() const override;
};

class UseQuery : public ASTNode {
public:
    std::string dbName;
    explicit UseQuery(const std::string& name);
    std::string toString() const override;
};

#endif // DBMS_PAIN_AST_H