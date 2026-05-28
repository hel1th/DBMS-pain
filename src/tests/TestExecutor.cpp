#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include "engine/Executor.h"
#include "parser/SqlParser.h"

// ---------- вспомогательные функции для работы с результатами ----------
static Value getValue(const Row& row, const std::string& colName) {
    for (const auto& [name, val]: row) {
        if (name == colName)
            return val;
    }
    return std::nullopt;
}

static int asInt(const Value& v) {
    if (!v.has_value())
        return 0;
    if (val::isInt(v))
        return val::getInt(v);
    return 0;
}

// ---------- тестовая инфраструктура с выводом ----------
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_CHECK(cond, msg)                                                                      \
    do {                                                                                           \
        if (cond) {                                                                                \
            std::cout << "[PASS] " << msg << std::endl;                                            \
            ++tests_passed;                                                                        \
        } else {                                                                                   \
            std::cout << "[FAIL] " << msg << std::endl;                                            \
            ++tests_failed;                                                                        \
            throw std::runtime_error(std::string("Test failed: ") + msg);                          \
        }                                                                                          \
    } while (0)

#define TEST_EQUAL_INT(val, expected, msg) TEST_CHECK((val) == (expected), msg)

#define TEST_TRUE(cond, msg) TEST_CHECK(cond, msg)

// Выполнение запроса с парсингом
static QueryResult run(Executor& ex, SqlParser& parser, const std::string& sql) {
    std::unique_ptr<ASTNode> node = parser.parse(sql);
    if (!node) {
        return {false, "Parser returned nullptr for: " + sql, {}, 0};
    }
    return ex.execute(node.get());
}

// ---------- базовый тест ----------
void testBasic() {
    std::cout << "\n=== testBasic ===" << std::endl;
    std::filesystem::remove_all("./data/testdb");
    std::filesystem::remove("./data/catalog.dat");

    Executor ex;
    SqlParser parser;

    auto r = run(ex, parser, "CREATE DATABASE testdb;");
    TEST_TRUE(r.ok, "CREATE DATABASE");
    r = run(ex, parser, "USE testdb;");
    TEST_TRUE(r.ok, "USE");
    r = run(ex, parser, "CREATE TABLE users (id INT INDEXED, name STRING NOT NULL, age INT);");
    TEST_TRUE(r.ok, "CREATE TABLE");
    r = run(ex, parser, "INSERT INTO users (id, name, age) VALUE (1, \"alice\", 30);");
    TEST_TRUE(r.ok && r.affected == 1, "INSERT");
    r = run(ex, parser, "SELECT * FROM users;");
    TEST_TRUE(r.ok && r.rows.size() == 1, "SELECT *");
    std::cout << ex.toJSON(r.rows) << std::endl;

    run(ex, parser, "DROP TABLE users;");
    run(ex, parser, "DROP DATABASE testdb;");
    std::cout << "=== testBasic PASSED ===" << std::endl;
}

// ---------- тест логических выражений (AND/OR, скобки, BETWEEN, LIKE) ----------
void testLogicalExpressions() {
    std::cout << "\n=== testLogicalExpressions ===" << std::endl;
    std::filesystem::remove_all("./data/testdb_logic");
    std::filesystem::remove("./data/catalog.dat");

    Executor ex;
    SqlParser parser;

    auto r = run(ex, parser, "CREATE DATABASE testdb_logic;");
    TEST_TRUE(r.ok, "CREATE DATABASE");
    r = run(ex, parser, "USE testdb_logic;");
    TEST_TRUE(r.ok, "USE");
    r = run(ex, parser, "CREATE TABLE people (id INT INDEXED, name STRING, age INT, salary INT);");
    TEST_TRUE(r.ok, "CREATE TABLE");

    r = run(ex, parser,
            "INSERT INTO people (id, name, age, salary) VALUE "
            "(1, \"Alice\", 25, 5000), "
            "(2, \"Bob\", 30, 6000), "
            "(3, \"Charlie\", 35, 7000), "
            "(4, \"David\", 40, 8000), "
            "(5, \"Eve\", 22, 4000);");
    TEST_TRUE(r.ok && r.affected == 5, "INSERT 5 rows");


    // AND
    r = run(ex, parser, "SELECT id FROM people WHERE age > 25 AND salary < 8000;");
    TEST_TRUE(r.ok && r.rows.size() == 2, "AND: age>25 AND salary<8000 -> 2 rows");


    // OR
    r = run(ex, parser, "SELECT id FROM people WHERE age < 23 OR age > 35;");
    TEST_TRUE(r.ok && r.rows.size() == 2, "OR: age<23 OR age>35 -> 2 rows");


    // Приоритет AND над OR
    r = run(ex, parser,
            "SELECT id FROM people WHERE age > 30 AND salary < 7500 OR name = \"Eve\";");
    TEST_TRUE(r.ok && r.rows.size() == 2, "AND over OR priority -> 2 rows");


    // Скобки меняют приоритет
    r = run(ex, parser,
            "SELECT id FROM people WHERE age > 30 AND (salary < 7500 OR name = \"Eve\");");
    TEST_TRUE(r.ok && r.rows.size() == 1, "Parentheses change priority -> 1 row");
    int id = asInt(getValue(r.rows[0], "id"));
    TEST_EQUAL_INT(id, 3, "Correct row id = 3 (Charlie)");


    // BETWEEN
    r = run(ex, parser, "SELECT id FROM people WHERE age BETWEEN 25 AND 35;");
    TEST_TRUE(r.ok && r.rows.size() == 3, "BETWEEN 25 AND 35 -> 3 rows");


    // LIKE (regex)
    r = run(ex, parser, "SELECT id FROM people WHERE name LIKE \"^A.*\";");
    TEST_TRUE(r.ok && r.rows.size() == 1 && asInt(getValue(r.rows[0], "id")) == 1,
              "LIKE regex '^A.*' -> Alice");


    // UPDATE с составным условием
    r = run(ex, parser, "UPDATE people SET salary = 9999 WHERE age >= 30 AND age <= 35;");
    std::cerr << "ok: " << r.ok << " affected: " << r.affected << " error: " << r.error << "\n";
    TEST_TRUE(r.ok && r.affected == 2, "UPDATE with AND: age between 30 and 35 -> 2 rows affected");
    r = run(ex, parser, "SELECT salary FROM people WHERE name = \"Bob\";");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "salary")) == 9999, "Bob salary updated to 9999");


    // DELETE с составным условием
    r = run(ex, parser, "DELETE FROM people WHERE age < 25 OR age > 35;");
    TEST_TRUE(r.ok && r.affected == 2, "DELETE with OR: age<25 OR age>35 -> 2 rows deleted");
    r = run(ex, parser, "SELECT COUNT(*) FROM people;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "COUNT(*)")) == 3, "Remaining rows = 3");

    run(ex, parser, "DROP TABLE people;");
    run(ex, parser, "DROP DATABASE testdb_logic;");
    std::cout << "=== testLogicalExpressions PASSED ===" << std::endl;
}

// ---------- тест агрегатных функций ----------
void testAggregateFunctions() {
    std::cout << "\n=== testAggregateFunctions ===" << std::endl;
    std::filesystem::remove_all("./data/testdb_agg");
    std::filesystem::remove("./data/catalog.dat");

    Executor ex;
    SqlParser parser;

    auto r = run(ex, parser, "CREATE DATABASE testdb_agg;");
    TEST_TRUE(r.ok, "CREATE DATABASE");
    r = run(ex, parser, "USE testdb_agg;");
    TEST_TRUE(r.ok, "USE");
    r = run(ex, parser, "CREATE TABLE sales (id INT, product STRING, price INT, quantity INT);");
    TEST_TRUE(r.ok, "CREATE TABLE");

    r = run(ex, parser,
            "INSERT INTO sales (id, product, price, quantity) VALUE "
            "(1, \"apple\", 100, 2), "
            "(2, \"banana\", 150, 3), "
            "(3, \"apple\", 100, 5), "
            "(4, \"cherry\", 200, 1);");
    TEST_TRUE(r.ok && r.affected == 4, "INSERT 4 rows");

    r = run(ex, parser, "SELECT COUNT(*) FROM sales;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "COUNT(*)")) == 4, "COUNT(*) = 4");
    r = run(ex, parser, "SELECT COUNT(*) FROM sales WHERE product = \"apple\";");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "COUNT(*)")) == 2, "COUNT(*) with WHERE = 2");
    r = run(ex, parser, "SELECT SUM(price) FROM sales;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "SUM(price)")) == 550, "SUM(price) = 550");
    r = run(ex, parser, "SELECT SUM(quantity) FROM sales WHERE product = \"apple\";");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "SUM(quantity)")) == 7,
              "SUM(quantity) with WHERE = 7");
    r = run(ex, parser, "SELECT AVG(price) FROM sales;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "AVG(price)")) == 137,
              "AVG(price) = 137 (int division)");
    r = run(ex, parser, "SELECT AVG(quantity) FROM sales WHERE price >= 150;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "AVG(quantity)")) == 2,
              "AVG(quantity) with WHERE = 2");


    // несколько агрегатов
    r = run(ex, parser,
            "SELECT COUNT(*), SUM(price), AVG(quantity) FROM sales WHERE product != \"apple\";");
    TEST_TRUE(r.ok && r.rows.size() == 1, "Multiple aggregates: row count 1");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "COUNT(*)")), 2, "COUNT(*) = 2");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "SUM(price)")), 350, "SUM(price) = 350");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "AVG(quantity)")), 2, "AVG(quantity) = 2");


    // алиасы
    r = run(ex, parser, "SELECT SUM(quantity) AS total_qty, AVG(price) AS avg_price FROM sales;");
    TEST_TRUE(r.ok, "Aggregates with AS");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "total_qty")), 11, "SUM(quantity) AS total_qty = 11");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "avg_price")), 137, "AVG(price) AS avg_price = 137");


    // пустая таблица
    r = run(ex, parser, "CREATE TABLE empty (x INT);");
    TEST_TRUE(r.ok, "CREATE empty table");
    r = run(ex, parser, "SELECT COUNT(*), SUM(x), AVG(x) FROM empty;");
    TEST_TRUE(r.ok && r.rows.size() == 1, "Aggregates on empty table return one row");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "COUNT(*)")), 0, "COUNT(*) = 0");

    run(ex, parser, "DROP TABLE sales;");
    run(ex, parser, "DROP TABLE empty;");
    run(ex, parser, "DROP DATABASE testdb_agg;");
    std::cout << "=== testAggregateFunctions PASSED ===" << std::endl;
}

// ---------- тест больших данных (500 строк) ----------
void testLargeData() {
    std::cout << "\n=== testLargeData (500 rows) ===" << std::endl;
    std::filesystem::remove_all("./data/testdb_large");
    std::filesystem::remove("./data/catalog.dat");

    Executor ex;
    SqlParser parser;

    auto r = run(ex, parser, "CREATE DATABASE testdb_large;");
    TEST_TRUE(r.ok, "CREATE DATABASE");
    r = run(ex, parser, "USE testdb_large;");
    TEST_TRUE(r.ok, "USE");
    r = run(ex, parser, "CREATE TABLE numbers (id INT, val INT, category INT);");
    TEST_TRUE(r.ok, "CREATE TABLE");

    // вставка 500 строк
    const int N = 500;
    std::string insertSQL = "INSERT INTO numbers (id, val, category) VALUE ";
    for (int i = 1; i <= N; ++i) {
        int val = i * 10;
        int cat = (i - 1) % 5;
        insertSQL += "(" + std::to_string(i) + ", " + std::to_string(val) + ", " +
                     std::to_string(cat) + ")";
        if (i < N)
            insertSQL += ", ";
    }
    insertSQL += ";";
    r = run(ex, parser, insertSQL);
    TEST_TRUE(r.ok && r.affected == N, "Insert 500 rows");

    // агрегаты на всех строках
    r = run(ex, parser, "SELECT COUNT(*), SUM(val), AVG(val) FROM numbers;");
    TEST_TRUE(r.ok, "Aggregates on full table");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "COUNT(*)")), N, "COUNT(*) = 500");
    long long expectedSum = 0;
    for (int i = 1; i <= N; ++i)
        expectedSum += i * 10;
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "SUM(val)")), expectedSum, "SUM(val) matches");
    int expectedAvg = static_cast<int>(static_cast<double>(expectedSum) / N);
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "AVG(val)")), expectedAvg, "AVG(val) matches (int)");

    // агрегаты с WHERE category = 2
    int cat2Count = 0;
    long long cat2Sum = 0;
    for (int i = 1; i <= N; ++i) {
        if ((i - 1) % 5 == 2) {
            cat2Count++;
            cat2Sum += i * 10;
        }
    }
    r = run(ex, parser, "SELECT COUNT(*), SUM(val), AVG(val) FROM numbers WHERE category == 2;");
    TEST_TRUE(r.ok, "Aggregates with WHERE category=2");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "COUNT(*)")), cat2Count,
                   "COUNT(*) matches category=2");
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "SUM(val)")), cat2Sum, "SUM(val) matches category=2");
    int cat2Avg = cat2Count ? static_cast<int>(static_cast<double>(cat2Sum) / cat2Count) : 0;
    TEST_EQUAL_INT(asInt(getValue(r.rows[0], "AVG(val)")), cat2Avg, "AVG(val) matches category=2");

    // UPDATE
    r = run(ex, parser, "UPDATE numbers SET val = 15 WHERE id == 1;");
    TEST_TRUE(r.ok && r.affected == 1, "UPDATE id=1 set val=15");
    r = run(ex, parser, "SELECT val FROM numbers WHERE id == 1;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "val")) == 15, "Updated val: id=1 now 15");

    // DELETE с составным условием
    int toDelete = 0;
    for (int i = 1; i <= N; ++i) {
        int curVal = i * 10;
        if ((i - 1) % 5 == 3 && curVal > 300)
            toDelete++;
    }
    r = run(ex, parser, "DELETE FROM numbers WHERE category = 3 AND val > 300;");
    TEST_TRUE(r.ok && r.affected == toDelete, "DELETE with AND condition");
    r = run(ex, parser, "SELECT COUNT(*) FROM numbers;");
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "COUNT(*)")) == N - toDelete,
              "Remaining rows count correct");

    // сложный OR со скобками
    r = run(ex, parser,
            "SELECT COUNT(*) FROM numbers WHERE (category = 1 AND val < 1000) OR (category = 4 "
            "AND val > 2000);");
    int expected = 0;
    for (int i = 1; i <= N; ++i) {
        int curVal = i * 10;
        if ((i - 1) % 5 == 1 && curVal < 1000)
            expected++;
        else if ((i - 1) % 5 == 4 && curVal > 2000)
            expected++;
    }
    TEST_TRUE(r.ok && asInt(getValue(r.rows[0], "COUNT(*)")) == expected,
              "Complex OR with parentheses");

    run(ex, parser, "DROP TABLE numbers;");
    run(ex, parser, "DROP DATABASE testdb_large;");
    std::cout << "=== testLargeData PASSED ===" << std::endl;
}

// ---------- точка входа ----------
int TestExecutor() {
    try {
        testBasic();
        testLogicalExpressions();
        testAggregateFunctions();
        testLargeData();
        std::cout << "\n=== ALL TESTS PASSED (" << tests_passed << " checks, " << tests_failed
                  << " failed) ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nEXCEPTION: " << e.what() << std::endl;
        std::cerr << "Tests passed: " << tests_passed << ", failed: " << tests_failed << std::endl;
        return 1;
    }
}


// helper
// void printAST(const ASTNode* node, int depth = 0) {
//     if (!node) {
//         std::cerr << std::string(depth * 2, ' ') << "NULL\n";
//         return;
//     }
//     std::string pad(depth * 2, ' ');
//     switch (node->kind) {
//         case NodeKind::AND_OP: {
//             std::cerr << pad << "AND\n";
//             auto* n = dynamic_cast<const AndOp*>(node);
//             printAST(n->left.get(), depth + 1);
//             printAST(n->right.get(), depth + 1);
//             break;
//         }
//         case NodeKind::OR_OP: {
//             std::cerr << pad << "OR\n";
//             auto* n = dynamic_cast<const OrOp*>(node);
//             printAST(n->left.get(), depth + 1);
//             printAST(n->right.get(), depth + 1);
//             break;
//         }
//         case NodeKind::BINARY_OP: {
//             auto* n = dynamic_cast<const BinaryOp*>(node);
//             std::cerr << pad << "BIN(" << n->op << ")\n";
//             printAST(n->left.get(), depth + 1);
//             printAST(n->right.get(), depth + 1);
//             break;
//         }
//         case NodeKind::COLUMN_REF:
//             std::cerr << pad << "COL(" << dynamic_cast<const ColumnRef*>(node)->name << ")\n";
//             break;
//         case NodeKind::LITERAL: {
//             auto* l = dynamic_cast<const Literal*>(node);
//             if (val::isInt(l->value))
//                 std::cerr << pad << "LIT(" << val::getInt(l->value) << ")\n";
//             else if (val::isString(l->value))
//                 std::cerr << pad << "LIT(\"" << val::getString(l->value) << "\")\n";
//             else
//                 std::cerr << pad << "LIT(NULL)\n";
//             break;
//         }
//         default:
//             std::cerr << pad << "NODE(" << (int) node->kind << ")\n";
//     }
// }
