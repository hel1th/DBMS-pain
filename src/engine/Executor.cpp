#include "Executor.h"

#include <regex>

Executor::Executor() : catalog_("./data") {
    for (const auto& name : catalog_.listDatabases()) {
        databases_[name] = std::make_unique<Database>(
            "./data/" + name, name
        );
    }
}

QueryResult Executor::execCreateDatabase(const CreateDatabaseQuery& q) {
    catalog_.addDatabase(q.dbName);
    databases_[q.dbName] = std::make_unique<Database>(
        "./data/" + q.dbName, q.dbName
    );

    return {true, "",{}, 0};
}

QueryResult Executor::execDropDatabase(const DropDatabaseQuery& q) {
    catalog_.removeDatabase(q.dbName);
    databases_.erase(q.dbName);
    if (currentDb_ == q.dbName)
        currentDb_ = "";
    return {true, "", {}, 0};
}

QueryResult Executor::execute(ASTNode* node) {
    if (!node)
        return {false, "Empty query"};

    switch (node->kind) {
        case NodeKind::INSERT_QUERY:
            return execInsert(*dynamic_cast<InsertQuery*>(node));
        case NodeKind::SELECT_QUERY:
            return execSelect(*dynamic_cast<SelectQuery*>(node));
        case NodeKind::UPDATE_QUERY:
            return execUpdate(*dynamic_cast<UpdateQuery*>(node));
        case NodeKind::DELETE_QUERY:
            return execDelete(*dynamic_cast<DeleteQuery*>(node));
        case NodeKind::CREATE_TABLE_QUERY:
            return execCreateTable(*dynamic_cast<CreateTableQuery*>(node));
        case NodeKind::DROP_TABLE_QUERY:
            return execDropTable(*dynamic_cast<DropTableQuery*>(node));
        case NodeKind::CREATE_DATABASE_QUERY:
            return execCreateDatabase(*dynamic_cast<CreateDatabaseQuery*>(node));
        case NodeKind::DROP_DATABASE_QUERY:
            return execDropDatabase(*dynamic_cast<DropDatabaseQuery*>(node));
        case NodeKind::USE_QUERY:
            return execUse(*dynamic_cast<UseQuery*>(node));
        default:
            return {false, "Empty query type"};
    }
}

//             exec funcs region
QueryResult Executor::execUse(const UseQuery& q) {
    if (!catalog_.hasDatabase(q.dbName))
        throw SemanticError("Database does not exist: " + q.dbName);
    currentDb_ = q.dbName;
    return {true, "", {}, 0};
}

Database& Executor::currentDatabase() {
    if (currentDb_.empty())
        throw SemanticError("No database selected. Use USE <db_name>");
    auto it = databases_.find(currentDb_);
    if (it == databases_.end())
        throw SemanticError("Database not loaded: " + currentDb_);
    return *it->second;
}

QueryResult Executor::execCreateTable(const CreateTableQuery& q) {
    Database& db = currentDatabase();
    Schema schema;
    schema.tableName = q.tableName;
    for (const auto& col : q.columns) {
        ColumnDef def;
        def.name    = col.name;
        def.type    = col.type;
        def.notNull  = col.notNull;
        def.indexed  = col.indexed;
        def.default_value = col.defaultValue;
        schema.columns.push_back(def);
    }
    db.createTable(schema);
    return {true, "", {}, 0};
}

QueryResult Executor::execDropTable(const DropTableQuery& q) {
    currentDatabase().dropTable(q.tableName);
    return {true, "", {}, 0};
}

QueryResult Executor::execInsert(const InsertQuery& q) {
    Database& db  = currentDatabase();
    Table&    tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;

    for (const auto& rowAst : q.values) {
        // собираем запись размером schema.columns.size()
        std::vector<Value> record(schema.columns.size(), std::nullopt);

        // заполняем переданные колонки
        for (size_t i = 0; i < q.columns.size(); i++) {
            int idx = schema.columnIndex(q.columns[i]);
            if (idx == -1)
                throw SemanticError("Unknown column: " + q.columns[i]);
            // rowAst[i] всегда Literal*
            auto* lit = dynamic_cast<const Literal*>(rowAst[i].get());
            if (!lit)
                throw SemanticError("Expected literal value in INSERT");
            record[idx] = lit->value;
        }

        // задание 10: DEFAULT и NOT_NULL для пропущенных колонок
        for (size_t i = 0; i < schema.columns.size(); i++) {
            if (!val::isNull(record[i]))
                continue; // значение уже передано

            const auto& col = schema.columns[i];
            if (col.default_value.has_value()) {
                record[i] = col.default_value.value();
            } else if (col.notNull) {
                throw SemanticError("Column '" + col.name +
                                    "' cannot be NULL");
            }
            // иначе остаётся NULL — это ок
        }

        tbl.insert(record);
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execDelete(const DeleteQuery& q) {
    Database& db  = currentDatabase();
    Table&    tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;
    std::vector<RecordID> toDelete;

    tbl.scan([&](RecordID recordID, const std::vector<Value>& record) {
        if (matches(record, schema, q.where.get()))
            toDelete.push_back(recordID);
    });

    for (auto recordID : toDelete) {
        tbl.remove(recordID);
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execUpdate(const UpdateQuery& q) {
    Database& db  = currentDatabase();
    Table&    tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;
    std::vector<std::pair<RecordID, std::vector<Value>>> toUpdate;

    tbl.scan([&](RecordID recordID, const std::vector<Value>& record) {
        if (!matches(record, schema, q.where.get()))
            return;

        std::vector<Value> newRecord = record;
        for (const auto& [colName, expr] : q.assignments) {
            int idx = schema.columnIndex(colName);
            if (idx == -1)
                throw SemanticError("Unknown column: " + colName);
            newRecord[idx] = resolve(expr.get(), record, schema);
        }
        toUpdate.emplace_back(recordID, std::move(newRecord));
    });

    for (auto& [recordID, newRecord] : toUpdate) {
        tbl.update(recordID, newRecord);
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execSelect(const SelectQuery& q) {
    Database& db  = currentDatabase();
    Table&    tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    // WHERE indexed_col == value
    if (int idxCol = schema.indexedColumn();
        idxCol != -1 && q.where && q.where->kind == NodeKind::BINARY_OP) {
        auto* bin = dynamic_cast<const BinaryOp*>(q.where.get());
        if (bin->op == "==") {
            bool leftIsCol = bin->left->kind  == NodeKind::COLUMN_REF;
            bool rightIsCol = bin->right->kind == NodeKind::COLUMN_REF;
            const ASTNode* colNode = leftIsCol  ? bin->left.get()  : bin->right.get();
            const ASTNode* valNode = leftIsCol  ? bin->right.get() : bin->left.get();

            if (!rightIsCol || !leftIsCol) {
                auto* ref = dynamic_cast<const ColumnRef*>(colNode);
                if (schema.columnIndex(ref->name) == idxCol) {
                    Value key = resolve(valNode, {}, schema);
                    try {
                        RecordID recordID = tbl.findByIndex(ref->name, key);
                        auto record  = tbl.fetch(recordID);
                        std::vector<Row> rows;
                        if (q.aggregates.empty())
                            rows.push_back(project(record, schema, q));
                        return {true, "", rows, 0};
                    } catch (const IndexError&) {
                        return {true, "", {}, 0}; // не найдено — пустой результат
                    }
                }
            }
        }
    }

    if (!q.aggregates.empty()) {
        // аккумуляторы для каждого агрегата
        struct Acc {
            double sum = 0;
            int    count = 0;
            bool   hasVal = false;
        };
        std::vector<Acc> accs(q.aggregates.size());

        tbl.scan([&](RecordID, const std::vector<Value>& record) {
            if (!matches(record, schema, q.where.get()))
                return;
            for (size_t i = 0; i < q.aggregates.size(); i++) {
                const auto& agg = q.aggregates[i];
                if (agg.func == "COUNT") {
                    accs[i].count++;
                } else {
                    int colIdx = schema.columnIndex(agg.column);
                    if (colIdx == -1)
                        throw SemanticError("Unknown column: " + agg.column);
                    const Value& v = record[colIdx];
                    if (!val::isNull(v)) {
                        double d = val::isInt(v) ? val::getInt(v) : 0;
                        accs[i].sum += d;
                        accs[i].count++;
                        accs[i].hasVal = true;
                    }
                }
            }
        });

        Row row;
        for (size_t i = 0; i < q.aggregates.size(); i++) {
            const auto& agg = q.aggregates[i];
            std::string label = agg.func + "(" + agg.column + ")";
            if (agg.func == "COUNT") {
                row.emplace_back(label, Value(accs[i].count));
            } else if (agg.func == "SUM") {
                row.emplace_back(label, Value(static_cast<int>(accs[i].sum)));
            } else if (agg.func == "AVG") {
                if (accs[i].count == 0)
                    row.emplace_back(label, std::nullopt);
                else
                    row.emplace_back(label,
                        Value(static_cast<int>(accs[i].sum / accs[i].count)));
            }
        }
        return {true, "", {row}, 0};
    }

    // обычный full scan
    std::vector<Row> rows;
    tbl.scan([&](RecordID, const std::vector<Value>& record) {
        if (matches(record, schema, q.where.get()))
            rows.push_back(project(record, schema, q));
    });

    return {true, "", rows, 0};
}


// Resolves Values for cmp ops
// For constansts aka Literals returns themselves Value(1)
// For column name aka ColumnRef id (example Record={id: 5} ) returns value of this column Value(5)
// If another Nodekind THROWS
// Operation: id == 1
Value Executor::resolve(const ASTNode* node, const std::vector<Value>& record,
                        const Schema& schema) {
    if (node->kind == NodeKind::LITERAL) {
        return dynamic_cast<const Literal*>(node)->value;
    }

    if (node->kind == NodeKind::COLUMN_REF) {
        auto* ref = dynamic_cast<const ColumnRef*>(node);

        const int idx = schema.columnIndex(ref->name);
        if (idx == -1)
            throw SemanticError("Unknown column: " + ref->name);

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
            auto* n = dynamic_cast<const OrOp*>(where);
            return matches(record, schema, n->left.get()) ||
                   matches(record, schema, n->right.get());
        }
        case NodeKind::AND_OP: {
            auto* n = dynamic_cast<const AndOp*>(where);
            return matches(record, schema, n->left.get()) &&
                   matches(record, schema, n->right.get());
        }
        case NodeKind::BINARY_OP: {
            auto* n = dynamic_cast<const BinaryOp*>(where);
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
            auto* n = dynamic_cast<const BetweenOp*>(where);
            Value low = resolve(n->low.get(), record, schema);
            Value high = resolve(n->high.get(), record, schema);
            Value val = resolve(n->expr.get(), record, schema);

            if (val::isNull(low) || val::isNull(val) || val::isNull(high))
                return false;

            // !(val < low) = (val >= low) = (low <= val)
            return !valueLess(val, low) && valueLess(val, high);
        }
        case NodeKind::LIKE_OP: {
            auto* n = dynamic_cast<const LikeOp*>(where);
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
            row.emplace_back(schema.columns[i].name, record[i]);

    } else {
        for (const auto& [name, alias] : q.columns) {
            const auto idx  = schema.columnIndex(name);

            if (idx == -1)
                throw SemanticError("Unknown column: " + name);

            auto nameOut = alias.empty() ? name : alias;
            row.emplace_back(nameOut, record[idx]);
        }
    }
    return row;
}
