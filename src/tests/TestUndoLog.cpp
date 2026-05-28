#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "undo-log/UndoLogManager.h"

// ----- Оператор вывода для вектора байт -----
std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0)
            os << ",";
        os << static_cast<int>(v[i]); // выводим как числа
    }
    os << "]";
    return os;
}

// ----- Оператор вывода для RevertActionType -----
std::ostream& operator<<(std::ostream& os, RevertActionType action) {
    switch (action) {
        case RevertActionType::REVERT_INSERT:
            return os << "REVERT_INSERT";
        case RevertActionType::REVERT_DELETE:
            return os << "REVERT_DELETE";
        case RevertActionType::REVERT_UPDATE:
            return os << "REVERT_UPDATE";
        default:
            return os << "UNKNOWN(" << static_cast<int>(action) << ")";
    }
}

// ----- Простой тестовый фреймворк -----
int tests_passed = 0;
int tests_failed = 0;

// Макросы теперь явно приводят оба аргумента к size_t для сравнения размеров.
// Для других типов (например, timeMS) оставляем auto, но для литералов используем static_cast.
#define ASSERT_EQ(expected, actual)                                                                \
    do {                                                                                           \
        auto exp_val = (expected);                                                                 \
        auto act_val = (actual);                                                                   \
        if (exp_val != act_val) {                                                                  \
            std::cerr << "[FAIL] Assertion failed: " << #expected << " == " << #actual << " (got " \
                      << act_val << ", expected " << exp_val << ")" << " at " << __FILE__ << ":"   \
                      << __LINE__ << std::endl;                                                    \
            tests_failed++;                                                                        \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define EXPECT_EQ(expected, actual)                                                                \
    do {                                                                                           \
        auto exp_val = (expected);                                                                 \
        auto act_val = (actual);                                                                   \
        if (exp_val != act_val) {                                                                  \
            std::cerr << "[WARN] Expectation failed: " << #expected << " == " << #actual           \
                      << " (got " << act_val << ", expected " << exp_val << ")" << " at "          \
                      << __FILE__ << ":" << __LINE__ << std::endl;                                 \
            tests_failed++;                                                                        \
        }                                                                                          \
    } while (0)

#define RUN_TEST(test_func)                                                                        \
    do {                                                                                           \
        std::cout << "Running " << #test_func << "... ";                                           \
        int old_failed = tests_failed;                                                             \
        test_func();                                                                               \
        if (tests_failed == old_failed) {                                                          \
            std::cout << "PASSED" << std::endl;                                                    \
            tests_passed++;                                                                        \
        } else {                                                                                   \
            std::cout << "FAILED" << std::endl;                                                    \
        }                                                                                          \
    } while (0)

const std::string TEST_FILE = "test_undo.wal";

void setup() { std::filesystem::remove(TEST_FILE); }

void teardown() { std::filesystem::remove(TEST_FILE); }

// ----- Тест 1: только вставки -----
void test_only_inserts() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> key1 = {1, 2, 3};
    std::vector<uint8_t> key2 = {4, 5, 6};

    lm.logUndoInsert("users", 1000, key1);
    lm.logUndoInsert("users", 1005, key2);

    auto records = lm.getRecordsToRevert("users", 1000);

    ASSERT_EQ(records.size(), static_cast<size_t>(2));
    EXPECT_EQ(records[0].timeMS, static_cast<size_t>(1005));
    EXPECT_EQ(records[0].actionType, RevertActionType::REVERT_INSERT);
    EXPECT_EQ(records[0].keys, key2);
    EXPECT_EQ(records[1].timeMS, static_cast<size_t>(1000));
    EXPECT_EQ(records[1].keys, key1);
}

// ----- Тест 2: только удаления -----
void test_only_deletes() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> key = {99};
    std::vector<uint8_t> oldData = {10, 20, 30, 40};

    lm.logUndoDelete("orders", 2000, key, oldData);

    auto records = lm.getRecordsToRevert("orders", 2000);

    ASSERT_EQ(records.size(), static_cast<size_t>(1));
    EXPECT_EQ(records[0].actionType, RevertActionType::REVERT_DELETE);
    EXPECT_EQ(records[0].keys, key);
    EXPECT_EQ(records[0].oldRowData, oldData);
}

// ----- Тест 3: только обновления -----
void test_only_updates() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> key = {5, 5};
    std::vector<uint8_t> oldData = {117, 117};

    lm.logUndoUpdate("products", 3000, key, oldData);

    auto records = lm.getRecordsToRevert("products", 3000);

    ASSERT_EQ(records.size(), static_cast<size_t>(1));
    EXPECT_EQ(records[0].actionType, RevertActionType::REVERT_UPDATE);
    EXPECT_EQ(records[0].keys, key);
    EXPECT_EQ(records[0].oldRowData, oldData);
}

// ----- Тест 4: несколько вставок и удалений -----
void test_multiple_inserts_deletes() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> k1 = {1}, k2 = {2}, k3 = {3};
    std::vector<uint8_t> data = {255};

    lm.logUndoInsert("items", 10, k1);
    lm.logUndoDelete("items", 20, k2, data);
    lm.logUndoInsert("items", 30, k3);

    auto records = lm.getRecordsToRevert("items", 10);

    ASSERT_EQ(records.size(), static_cast<size_t>(3));
    EXPECT_EQ(records[0].actionType, RevertActionType::REVERT_INSERT);
    EXPECT_EQ(records[1].actionType, RevertActionType::REVERT_DELETE);
    EXPECT_EQ(records[2].actionType, RevertActionType::REVERT_INSERT);
}

// ----- Тест 5: комбинация всех типов -----
void test_combination_all() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> k = {0};
    std::vector<uint8_t> d = {128};

    lm.logUndoInsert("logs", 100, k);
    lm.logUndoUpdate("logs", 200, k, d);
    lm.logUndoDelete("logs", 300, k, d);

    auto records = lm.getRecordsToRevert("logs", 100);

    ASSERT_EQ(records.size(), static_cast<size_t>(3));
    EXPECT_EQ(records[0].actionType, RevertActionType::REVERT_DELETE);
    EXPECT_EQ(records[1].actionType, RevertActionType::REVERT_UPDATE);
    EXPECT_EQ(records[2].actionType, RevertActionType::REVERT_INSERT);
}

// ----- Тест 6: фильтрация по времени -----
void test_timestamp_filtering() {
    UndoLogManager lm(TEST_FILE);
    std::vector<uint8_t> k = {1};

    lm.logUndoInsert("data", 1000, k);
    lm.logUndoInsert("data", 2000, k);
    lm.logUndoInsert("data", 3000, k);

    auto records = lm.getRecordsToRevert("data", 2000);

    ASSERT_EQ(records.size(), static_cast<size_t>(2));
    EXPECT_EQ(records[0].timeMS, static_cast<size_t>(3000));
    EXPECT_EQ(records[1].timeMS, static_cast<size_t>(2000));
}

// ----- Тест 7: обрезание лога -----
void test_truncate_log() {
    {
        UndoLogManager lm(TEST_FILE);
        std::vector<uint8_t> k = {1};
        lm.logUndoInsert("table", 1000, k);
        lm.logUndoInsert("table", 2000, k);
        lm.logUndoInsert("table", 3000, k);
        lm.truncateLog(2000);
    }
    UndoLogManager lm2(TEST_FILE);
    auto records = lm2.getRecordsToRevert("table", 1000);
    ASSERT_EQ(records.size(), static_cast<size_t>(1));
    EXPECT_EQ(records[0].timeMS, static_cast<size_t>(1000));
}

// ----- main -----
int testUndo() {
    RUN_TEST(test_only_inserts);
    teardown();

    RUN_TEST(test_only_deletes);
    teardown();

    RUN_TEST(test_only_updates);
    teardown();

    RUN_TEST(test_multiple_inserts_deletes);
    teardown();

    RUN_TEST(test_combination_all);
    teardown();

    RUN_TEST(test_timestamp_filtering);
    teardown();

    RUN_TEST(test_truncate_log);
    teardown();

    std::cout << "\n===== Summary: " << tests_passed << " passed, " << tests_failed
              << " failed =====" << std::endl;
    return tests_failed == 0 ? 0 : 1;
}
