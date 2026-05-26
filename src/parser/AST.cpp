#include "parser/AST.h"
#include <sstream>
#include <string>
#include <utility>

ASTNode::ASTNode(NodeKind k) : kind(k) {}

// листовые

Star::Star() : ASTNode(NodeKind::STAR) {}
std::string Star::toString() const { return "*"; }

ColumnRef::ColumnRef(const std::string& n) : ASTNode(NodeKind::COLUMN_REF), name(n) {}
std::string ColumnRef::toString() const { return "Column(" + name + ")"; }

Literal::Literal(Value v) : ASTNode(NodeKind::LITERAL), value(std::move(v)) {}

std::string Literal::toString() const {
    if (isNull())
        return "NULL";
    if (isInt())
        return "Int(" + std::to_string(asInt()) + ")";
    if (isString())
        return "String(\"" + asString() + "\")";
    return "?";
}

// операторы

BinaryOp::BinaryOp(const std::string& o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) :
    ASTNode(NodeKind::BINARY_OP), op(o), left(std::move(l)), right(std::move(r)) {}

std::string BinaryOp::toString() const {
    return "(" + left->toString() + " " + op + " " + right->toString() + ")";
}

AndOp::AndOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) :
    ASTNode(NodeKind::AND_OP), left(std::move(l)), right(std::move(r)) {}
std::string AndOp::toString() const {
    return "(" + left->toString() + " AND " + right->toString() + ")";
}

OrOp::OrOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) :
    ASTNode(NodeKind::OR_OP), left(std::move(l)), right(std::move(r)) {}
std::string OrOp::toString() const {
    return "(" + left->toString() + " OR " + right->toString() + ")";
}

BetweenOp::BetweenOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> l,
                     std::unique_ptr<ASTNode> h) :
    ASTNode(NodeKind::BETWEEN_OP), expr(std::move(e)), low(std::move(l)), high(std::move(h)) {}
std::string BetweenOp::toString() const {
    return "(BETWEEN " + expr->toString() + " " + low->toString() + " " + high->toString() + ")";
}

LikeOp::LikeOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> p) :
    ASTNode(NodeKind::LIKE_OP), expr(std::move(e)), pattern(std::move(p)) {}
std::string LikeOp::toString() const {
    return "(LIKE " + expr->toString() + " " + pattern->toString() + ")";
}

// команды

SelectQuery::SelectQuery() : ASTNode(NodeKind::SELECT_QUERY), star(false), where(nullptr) {}

std::string SelectQuery::toString() const {
    std::string res = "SELECT ";
    if (!aggregates.empty()) {
        for (size_t i = 0; i < aggregates.size(); ++i) {
            if (i > 0)
                res += ", ";
            res += aggregates[i].func + "(" + aggregates[i].column + ")";
        }
    } else if (star) {
        res += "*";
    } else {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0)
                res += ", ";
            res += columns[i].name;
            if (!columns[i].alias.empty())
                res += " AS " + columns[i].alias;
        }
    }
    res += " FROM " + tableName;
    if (where)
        res += " WHERE " + where->toString();
    return res;
}

InsertQuery::InsertQuery() : ASTNode(NodeKind::INSERT_QUERY) {}

std::string InsertQuery::toString() const {
    std::string res = "INSERT INTO " + tableName;
    if (!columns.empty()) {
        res += " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0)
                res += ", ";
            res += columns[i];
        }
        res += ")";
    }
    res += " VALUE ";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            res += ", ";
        res += "(";
        for (size_t j = 0; j < values[i].size(); ++j) {
            if (j > 0)
                res += ", ";
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
        if (i > 0)
            res += ", ";
        res += assignments[i].first + " = " + assignments[i].second->toString();
    }
    if (where)
        res += " WHERE " + where->toString();
    return res;
}

DeleteQuery::DeleteQuery() : ASTNode(NodeKind::DELETE_QUERY), where(nullptr) {}

std::string DeleteQuery::toString() const {
    std::string res = "DELETE FROM " + tableName;
    if (where)
        res += " WHERE " + where->toString();
    return res;
}

CreateTableQuery::CreateTableQuery() : ASTNode(NodeKind::CREATE_TABLE_QUERY) {}

std::string CreateTableQuery::toString() const {
    std::string res = "CREATE TABLE " + tableName + " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0)
            res += ", ";
        const auto& c = columns[i];
        res += c.name + " ";
        res += (c.type == ColType::INT ? "INT" : "STRING");
        if (c.notNull)
            res += " NOT_NULL";
        if (c.indexed)
            res += " INDEXED";
        if (!val::isNull(c.defaultValue)) {
            res += " DEFAULT ";
            if (val::isInt(c.defaultValue))
                res += std::to_string(val::getInt(c.defaultValue));
            else
                res += "\"" + val::getString(c.defaultValue) + "\"";
        }
    }
    res += ")";
    return res;
}

DropTableQuery::DropTableQuery(const std::string& name) :
    ASTNode(NodeKind::DROP_TABLE_QUERY), tableName(name) {}
std::string DropTableQuery::toString() const { return "DROP TABLE " + tableName; }

CreateDatabaseQuery::CreateDatabaseQuery(const std::string& name) :
    ASTNode(NodeKind::CREATE_DATABASE_QUERY), dbName(name) {}
std::string CreateDatabaseQuery::toString() const { return "CREATE DATABASE " + dbName; }

DropDatabaseQuery::DropDatabaseQuery(const std::string& name) :
    ASTNode(NodeKind::DROP_DATABASE_QUERY), dbName(name) {}
std::string DropDatabaseQuery::toString() const { return "DROP DATABASE " + dbName; }

UseQuery::UseQuery(const std::string& name) : ASTNode(NodeKind::USE_QUERY), dbName(name) {}
std::string UseQuery::toString() const { return "USE " + dbName; }
