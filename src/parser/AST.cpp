#include "AST.h"
#include <sstream>
#include <utility>

ASTNode::ASTNode(NodeKind k) : kind(k) {}

Star::Star() : ASTNode(NodeKind::STAR) {}
std::string Star::toString() const { return "*"; }

ColumnRef::ColumnRef(const std::string& n) : ASTNode(NodeKind::COLUMN_REF), name(n) {}
std::string ColumnRef::toString() const { return "Column(" + name + ")"; }

Literal::Literal(Value v) : ASTNode(NodeKind::LITERAL), value(std::move(v)) {}

std::string Literal::toString() const {
    if (!value.has_value()) return "NULL";
    if (std::holds_alternative<int>(*value))
        return "Int(" + std::to_string(std::get<int>(*value)) + ")";
    if (std::holds_alternative<std::string>(*value))
        return "String(\"" + std::get<std::string>(*value) + "\")";
    return "?";
}

bool Literal::isNull() const { return !value.has_value(); }
bool Literal::isInt() const { return value.has_value() && std::holds_alternative<int>(*value); }
bool Literal::isString() const { return value.has_value() && std::holds_alternative<std::string>(*value); }
int Literal::asInt() const { return std::get<int>(*value); }
const std::string& Literal::asString() const { return std::get<std::string>(*value); }

BinaryOp::BinaryOp(const std::string& o,
                   std::unique_ptr<ASTNode> l,
                   std::unique_ptr<ASTNode> r)
    : ASTNode(NodeKind::BINARY_OP), op(o), left(std::move(l)), right(std::move(r)) {}

std::string BinaryOp::toString() const {
    return "(" + left->toString() + " " + op + " " + right->toString() + ")";
}

AndOp::AndOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : ASTNode(NodeKind::AND_OP), left(std::move(l)), right(std::move(r)) {}

std::string AndOp::toString() const {
    return "(AND " + left->toString() + " " + right->toString() + ")";
}

OrOp::OrOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : ASTNode(NodeKind::OR_OP), left(std::move(l)), right(std::move(r)) {}

std::string OrOp::toString() const {
    return "(OR " + left->toString() + " " + right->toString() + ")";
}

BetweenOp::BetweenOp(std::unique_ptr<ASTNode> e,
                     std::unique_ptr<ASTNode> l,
                     std::unique_ptr<ASTNode> h)
    : ASTNode(NodeKind::BETWEEN_OP), expr(std::move(e)), low(std::move(l)), high(std::move(h)) {}

std::string BetweenOp::toString() const {
    return "(BETWEEN " + expr->toString() + " " + low->toString() + " " + high->toString() + ")";
}

LikeOp::LikeOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> p)
    : ASTNode(NodeKind::LIKE_OP), expr(std::move(e)), pattern(std::move(p)) {}

std::string LikeOp::toString() const {
    return "(LIKE " + expr->toString() + " " + pattern->toString() + ")";
}

SelectQuery::SelectQuery() : ASTNode(NodeKind::SELECT_QUERY), where(nullptr) {}

std::string SelectQuery::toString() const {
    std::string res = "SELECT ";
    if (aggregates.empty()) {
        if (columns.empty()) res += "*";
        else {
            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0) res += ", ";
                res += columns[i];
            }
        }
    } else {
        for (size_t i = 0; i < aggregates.size(); ++i) {
            if (i > 0) res += ", ";
            res += aggregates[i].func + "(" + aggregates[i].column + ")";
        }
    }
    res += " FROM " + tableName;
    if (!alias.empty()) res += " AS " + alias;
    if (where) res += " WHERE " + where->toString();
    return res;
}

InsertQuery::InsertQuery() : ASTNode(NodeKind::INSERT_QUERY) {}

std::string InsertQuery::toString() const {
    std::string res = "INSERT INTO " + tableName;
    if (!columns.empty()) {
        res += " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) res += ", ";
            res += columns[i];
        }
        res += ")";
    }
    res += " VALUES ";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) res += ", ";
        res += "(";
        for (size_t j = 0; j < values[i].size(); ++j) {
            if (j > 0) res += ", ";
            res += values[i][j]->toString();
        }
        res += ")";
    }
    return res;
}

UpdateQuery::UpdateQuery() : ASTNode(NodeKind::UPDATE_QUERY), where(nullptr) {}

std::string UpdateQuery::toString() const {
    std::string res = "UPDATE " + tableName + " SET ";
    for (size_t i = 0; i < assignments.size(); ++i) {
        if (i > 0) res += ", ";
        res += assignments[i].first + " = " + assignments[i].second->toString();
    }
    if (where) res += " WHERE " + where->toString();
    return res;
}

DeleteQuery::DeleteQuery() : ASTNode(NodeKind::DELETE_QUERY), where(nullptr) {}

std::string DeleteQuery::toString() const {
    std::string res = "DELETE FROM " + tableName;
    if (where) res += " WHERE " + where->toString();
    return res;
}

CreateTableQuery::CreateTableQuery() : ASTNode(NodeKind::CREATE_TABLE_QUERY) {}

std::string CreateTableQuery::toString() const {
    std::string res = "CREATE TABLE " + tableName + " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) res += ", ";
        res += columns[i].name + " " + columns[i].type;
        if (columns[i].notNull) res += " NOT NULL";
        if (columns[i].indexed) res += " INDEXED";
        if (columns[i].defaultValue.has_value()) {
            res += " DEFAULT ";
            if (!columns[i].defaultValue.has_value()) res += "NULL";
            else if (std::holds_alternative<int>(*columns[i].defaultValue))
                res += std::to_string(std::get<int>(*columns[i].defaultValue));
            else
                res += "\"" + std::get<std::string>(*columns[i].defaultValue) + "\"";
        }
    }
    res += ")";
    return res;
}

DropTableQuery::DropTableQuery(const std::string& name)
    : ASTNode(NodeKind::DROP_TABLE_QUERY), tableName(name) {}

std::string DropTableQuery::toString() const {
    return "DROP TABLE " + tableName;
}

CreateDatabaseQuery::CreateDatabaseQuery(const std::string& name)
    : ASTNode(NodeKind::CREATE_DATABASE_QUERY), dbName(name) {}

std::string CreateDatabaseQuery::toString() const {
    return "CREATE DATABASE " + dbName;
}

DropDatabaseQuery::DropDatabaseQuery(const std::string& name)
    : ASTNode(NodeKind::DROP_DATABASE_QUERY), dbName(name) {}

std::string DropDatabaseQuery::toString() const {
    return "DROP DATABASE " + dbName;
}

UseQuery::UseQuery(const std::string& name)
    : ASTNode(NodeKind::USE_QUERY), dbName(name) {}

std::string UseQuery::toString() const {
    return "USE " + dbName;
}