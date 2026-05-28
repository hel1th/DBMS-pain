#include <cassert>
#include <filesystem>
#include <iostream>
#include "engine/Executor.h"
#include "parser/SqlParser.h"

QueryResult run(Executor& ex, SqlParser& parser, const std::string& sql) {
    std::unique_ptr<ASTNode> node = parser.parse(sql);
    if (!node) {
        std::cerr << "Parser returned nullptr for: " << sql << std::endl;
        return {false, "Parser returned nullptr", {}, 0};
    }
    return ex.execute(node.get());
}

void testBasic() {
    // чистим остатки от предыдущего запуска
    std::filesystem::remove_all("./data/testdb");
    std::filesystem::remove("./data/catalog.dat");

    std::cout << "=== testBasic ===" << std::endl;
    Executor ex;
    SqlParser parser;

    // CREATE DATABASE
    auto r1 = run(ex, parser, "CREATE DATABASE testdb;");
    if (!r1.ok)
        std::cerr << "CREATE DATABASE error: " << r1.error << std::endl;
    assert(r1.ok && "CREATE DATABASE failed");
    std::cout << "CREATE DATABASE ok" << std::endl;

    // USE
    auto r2 = run(ex, parser, "USE testdb;");
    if (!r2.ok)
        std::cerr << "USE error: " << r2.error << std::endl;
    assert(r2.ok && "USE failed");
    std::cout << "USE ok" << std::endl;

    // CREATE TABLE
    auto r3 =
            run(ex, parser, "CREATE TABLE users (id INT INDEXED, name STRING NOT NULL, age INT);");
    if (!r3.ok)
        std::cerr << "CREATE TABLE error: " << r3.error << std::endl;
    assert(r3.ok && "CREATE TABLE failed");
    std::cout << "CREATE TABLE ok" << std::endl;

    // INSERT
    auto r4 = run(ex, parser, "INSERT INTO users (id, name, age) VALUE (1, \"alice\", 30);");
    if (!r4.ok)
        std::cerr << "INSERT error: " << r4.error << std::endl;
    assert(r4.ok && "INSERT failed");
    assert(r4.affected == 1);
    std::cout << "INSERT ok" << std::endl;

    // SELECT *
    auto r5 = run(ex, parser, "SELECT * FROM users;");
    if (!r5.ok)
        std::cerr << "SELECT error: " << r5.error << std::endl;
    assert(r5.ok && "SELECT failed");
    assert(r5.rows.size() == 1 && "Expected 1 row");
    std::cout << "SELECT ok, rows: " << r5.rows.size() << std::endl;
    std::cout << ex.toJSON(r5.rows) << std::endl;

    // CLEANUP
    run(ex, parser, "DROP TABLE users;");
    run(ex, parser, "DROP DATABASE testdb;");

    std::cout << "=== testBasic PASSED ===" << std::endl;
}

int TestExecutor() {
    try {
        testBasic();
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
