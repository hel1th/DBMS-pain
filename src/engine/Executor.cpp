#include "Executor.h"

#include <regex>


QueryResult Executor::execute(ASTNode* node) {
    if (!node)
        return {false, "Empty query"};

    switch (node->kind) {
        case NodeKind::INSERT_QUERY:
            return execInsert(*static_cast<InsertQuery*>(node));
        case NodeKind::SELECT_QUERY:
            return execSelect(*static_cast<SelectQuery*>(node));
        case NodeKind::UPDATE_QUERY:
            return execUpdate(*static_cast<UpdateQuery*>(node));
        case NodeKind::DELETE_QUERY:
            return execDelete(*static_cast<DeleteQuery*>(node));
        case NodeKind::CREATE_TABLE_QUERY:
            return execCreateTable(*static_cast<CreateTableQuery*>(node));
        case NodeKind::DROP_TABLE_QUERY:
            return execDropTable(*static_cast<DropTableQuery*>(node));
        case NodeKind::CREATE_DATABASE_QUERY:
            return execCreateDatabase(*static_cast<CreateDatabaseQuery*>(node));
        case NodeKind::DROP_DATABASE_QUERY:
            return execCreateDatabase(*static_cast<CreateDatabaseQuery*>(node));
        case NodeKind::USE_QUERY:
            return execUse(*static_cast<UseQuery*>(node));
        default:
            return {false, "Empty query type"};
    }
}

// Resolves Values for cmp ops
// For constansts aka Literals returns themselves Value(1)
// For column name aka ColumnRef id (example Record={id: 5} ) returns value of this column Value(5)
// If another Nodekind THROWS
// Operation: id == 1
Value Executor::resolve(const ASTNode* node, const std::vector<Value>& record,
                        const Schema& schema) {
    if (node->kind == NodeKind::LITERAL) {
        return static_cast<const Literal*>(node)->value;
    }

    if (node->kind == NodeKind::COLUMN_REF) {
        auto* ref = static_cast<const ColumnRef*>(node);
        const int idx = schema.columnIndex(ref->name);
        return record[idx];
    }

    throw SemanticError("Unexpected node type in expression");
}

// returns wether expression ( OR AND binary{==, >= ...} BETWEEN LIKE ) is true or false
bool Executor::matches(const std::vector<Value>& record, const Schema& schema,
                       const ASTNode* where) {
    if (!where)
        return true;

    switch (where->kind) {
        case NodeKind::OR_OP: {
            auto* n = static_cast<const OrOp*>(where);
            return matches(record, schema, n->left.get()) ||
                   matches(record, schema, n->right.get());
        }
        case NodeKind::AND_OP: {
            auto* n = static_cast<const AndOp*>(where);
            return matches(record, schema, n->left.get()) &&
                   matches(record, schema, n->right.get());
        }
        case NodeKind::BINARY_OP: {
            auto* n = static_cast<const BinaryOp*>(where);
            Value lv = resolve(n->left.get(), record, schema);
            Value rv = resolve(n->right.get(), record, schema);

            if (val::isNull(lv) || val::isNull(rv))
                return false;

            if (n->op == "==")
                return valueEqual(lv, rv);
            if (n->op == "!=")
                return !valueEqual(lv, rv);
            if (n->op == "<")
                return valueLess(lv, rv);
            if (n->op == ">")
                return valueLess(rv, lv);
            if (n->op == "<=")
                return !valueLess(rv, lv);
            if (n->op == ">=")
                return !valueLess(lv, rv);

            throw SemanticError("Unknown operator: " + n->op);
        }

        case NodeKind::BETWEEN_OP: {
            // val BETWEEN low AND high  ->  low <= val < high
            auto* n = static_cast<const BetweenOp*>(where);
            Value low = resolve(n->low.get(), record, schema);
            Value high = resolve(n->high.get(), record, schema);
            Value val = resolve(n->expr.get(), record, schema);

            if (val::isNull(low) || val::isNull(val) || val::isNull(high))
                return false;

            // !(val < low) = (val >= low) = (low <= val)
            return !valueLess(val, low) && valueLess(val, high);
        }
        case NodeKind::LIKE_OP: {
            auto* n = static_cast<const LikeOp*>(where);
            Value val     = resolve(n->expr.get(),    record, schema);
            Value pattern = resolve(n->pattern.get(), record, schema);

            if (val::isNull(val) || val::isNull(pattern)) return false;

            if (!val::isString(val) || !val::isString(pattern))
                throw SemanticError("LIKE requires string operands");

            // используем std::regex для сопоставления
            try {
                std::regex re(val::getString(pattern));
                return std::regex_match(val::getString(val), re);
            } catch (const std::regex_error&) {
                throw SemanticError("Invalid regex pattern: "
                                    + val::getString(pattern));
            }
        }
        default:
            throw SemanticError("Unexpected node type in WHERE clause");
    }
}


Row Executor::project(const std::vector<Value>& record, const Schema& schema,
                      const SelectQuery& q) {
    Row row;

    if (q.star) {
        for (size_t i = 0; i < schema.columns.size(); i++)
            row.push_back({schema.columns[i].name, record[i]});

    } else {
        for (auto& col : q.columns) {
            auto idx  = schema.columnIndex(col.name);
            auto nameOut = col.alias.empty() ? col.alias : col.name;
            row.push_back({nameOut, record[idx]});
        }
    }
    return row;
}
