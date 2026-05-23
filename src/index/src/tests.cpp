#include "tests.h"
#include "BStarPlusTree.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <string>

// ----------------------------------------------------------------------------
// Вспомогательные макросы для отладки
// ----------------------------------------------------------------------------

// Вывод сообщения об ошибке и содержимого дерева (если дерево определено)
#define CHECK(condition, tree, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "\n[ERROR] " << message << "\n"; \
            std::cerr << "Condition failed: " << #condition << "\n"; \
            std::cerr << "File: " << __FILE__ << ", line: " << __LINE__ << "\n"; \
            tree.printStructure(); \
            abort(); \
        } \
    } while(0)

// Для случаев, когда дерево не определено (просто assert с сообщением)
#define CHECK_SIMPLE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "\n[ERROR] " << message << "\n"; \
            std::cerr << "Condition failed: " << #condition << "\n"; \
            std::cerr << "File: " << __FILE__ << ", line: " << __LINE__ << "\n"; \
            abort(); \
        } \
    } while(0)

// Сравнение двух значений с выводом ожидаемого и реального
#define CHECK_EQ(actual, expected, tree, context) \
    do { \
        if ((actual) != (expected)) { \
            std::cerr << "\n[ERROR] " << context << "\n"; \
            std::cerr << "Expected: " << (expected) << ", got: " << (actual) << "\n"; \
            std::cerr << "File: " << __FILE__ << ", line: " << __LINE__ << "\n"; \
            tree.printStructure(); \
            abort(); \
        } \
    } while(0)

// ----------------------------------------------------------------------------
// Helper: проверка, что дерево содержит ровно expected элементы в правильном порядке
// ----------------------------------------------------------------------------
template<typename Tree, typename K, typename V>
void checkTreeContent(const Tree& tree, const std::vector<std::pair<K, V>>& expected) {
    CHECK_EQ(tree.size(), expected.size(), tree, "Tree size mismatch");
    auto it = tree.cbegin();
    for (size_t i = 0; i < expected.size(); ++i, ++it) {
        CHECK(it != tree.cend(), tree, "Tree ended prematurely");
        CHECK_EQ(it->first, expected[i].first, tree, "Key mismatch at index " + std::to_string(i));
        CHECK_EQ(it->second, expected[i].second, tree, "Value mismatch at index " + std::to_string(i));
    }
    CHECK(it == tree.cend(), tree, "Tree has extra elements after expected end");
}

// ============================================================================
// Test 1: Default constructor – empty tree
// ============================================================================
void testDefaultConstructor() {
    std::cout << "Test 1: Default constructor\n";
    BspTree<int, int> tree;
    CHECK_SIMPLE(tree.empty(), "Tree should be empty");
    CHECK_SIMPLE(tree.size() == 0, "Tree size should be 0");
    CHECK_SIMPLE(tree.begin() == tree.end(), "begin() should equal end()");
    CHECK_SIMPLE(tree.cbegin() == tree.cend(), "cbegin() should equal cend()");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 2: Constructor with comparator and allocator
// ============================================================================
void testComparatorAllocatorCtor() {
    std::cout << "Test 2: Constructor with comparator and allocator\n";
    std::greater<int> cmp;
    pp_allocator<std::pair<const int, int>> alloc;
    BspTree<int, int, std::greater<int>> tree(cmp, alloc);
    CHECK_SIMPLE(tree.empty(), "Tree should be empty");
    tree.insert({10, 100});
    tree.insert({5, 50});
    auto it = tree.begin();
    CHECK_EQ(it->first, 10, tree, "First key should be 10 (descending order)");
    ++it;
    CHECK_EQ(it->first, 5, tree, "Second key should be 5");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 3: Iterator range constructor
// ============================================================================
void testRangeConstructor() {
    std::cout << "Test 3: Iterator range constructor\n";
    std::vector<std::pair<int, std::string>> data = {{3, "c"}, {1, "a"}, {4, "d"}, {2, "b"}};
    BspTree<int, std::string> tree(data.begin(), data.end());
    std::vector<std::pair<int, std::string>> expected = {{1, "a"}, {2, "b"}, {3, "c"}, {4, "d"}};
    checkTreeContent(tree, expected);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 4: Initializer list constructor
// ============================================================================
void testInitializerListCtor() {
    std::cout << "Test 4: Initializer list constructor\n";
    BspTree<int, double> tree = {{2, 2.2}, {1, 1.1}, {3, 3.3}};
    std::vector<std::pair<int, double>> expected = {{1, 1.1}, {2, 2.2}, {3, 3.3}};
    checkTreeContent(tree, expected);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 5: Copy constructor
// ============================================================================
void testCopyConstructor() {
    std::cout << "Test 5: Copy constructor\n";
    BspTree<int, int> original;
    for (int i = 0; i < 20; ++i) original.insert({i, i * 10});
    BspTree<int, int> copy(original);
    CHECK_EQ(original.size(), copy.size(), original, "Copy size mismatch");
    auto it1 = original.cbegin();
    auto it2 = copy.cbegin();
    while (it1 != original.cend()) {
        CHECK_EQ(it1->first, it2->first, original, "Copy key mismatch");
        CHECK_EQ(it1->second, it2->second, original, "Copy value mismatch");
        ++it1; ++it2;
    }
    CHECK(it2 == copy.cend(), copy, "Copy has extra elements");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 6: Move constructor
// ============================================================================
void testMoveConstructor() {
    std::cout << "Test 6: Move constructor\n";
    BspTree<int, int> original;
    for (int i = 0; i < 10; ++i) original.insert({i, i});
    size_t oldSize = original.size();
    BspTree<int, int> moved(std::move(original));
    CHECK_EQ(moved.size(), oldSize, moved, "Moved tree size mismatch");
    CHECK_SIMPLE(original.empty(), "Original tree should be empty after move");
    CHECK_SIMPLE(original.begin() == original.end(), "Original begin() != end() after move");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 7: Copy assignment
// ============================================================================
void testCopyAssignment() {
    std::cout << "Test 7: Copy assignment\n";
    BspTree<int, int> a, b;
    for (int i = 0; i < 5; ++i) a.insert({i, i});
    for (int i = 5; i < 10; ++i) b.insert({i, i});
    a = b;
    CHECK_EQ(a.size(), b.size(), a, "Copy assignment size mismatch");
    auto ita = a.cbegin();
    auto itb = b.cbegin();
    while (ita != a.cend()) {
        CHECK_EQ(ita->first, itb->first, a, "Copy assignment key mismatch");
        ++ita; ++itb;
    }
    CHECK(itb == b.cend(), b, "Copy assignment - extra elements");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 8: Move assignment
// ============================================================================
void testMoveAssignment() {
    std::cout << "Test 8: Move assignment\n";
    BspTree<int, int> a, b;
    for (int i = 0; i < 8; ++i) a.insert({i, i});
    size_t aSize = a.size();
    b = std::move(a);
    CHECK_EQ(b.size(), aSize, b, "Move assignment size mismatch");
    CHECK_SIMPLE(a.empty(), "Source tree should be empty after move assignment");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 9: Insert single element and check basic access
// ============================================================================
void testInsertBasic() {
    std::cout << "Test 9: Insert basic\n";
    BspTree<int, int> tree;
    auto [it, inserted] = tree.insert({42, 100});
    CHECK(inserted, tree, "Insert should return true");
    CHECK_EQ(it->first, 42, tree, "Inserted key mismatch");
    CHECK_EQ(it->second, 100, tree, "Inserted value mismatch");
    CHECK_EQ(tree.size(), 1, tree, "Size should be 1");
    CHECK(tree.contains(42), tree, "contains(42) should be true");
    CHECK(tree.find(42) != tree.end(), tree, "find(42) should not be end");
    CHECK_EQ(tree.at(42), 100, tree, "at(42) value mismatch");
    CHECK_EQ(tree[42], 100, tree, "operator[] value mismatch");
    tree[42] = 200;
    CHECK_EQ(tree.at(42), 200, tree, "After assignment, at(42) should be 200");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 10: Insert duplicate key – should fail
// ============================================================================
void testInsertDuplicate() {
    std::cout << "Test 10: Insert duplicate\n";
    BspTree<int, int> tree;
    tree.insert({1, 10});
    auto [it, inserted] = tree.insert({1, 20});
    CHECK(!inserted, tree, "Duplicate insert should return false");
    CHECK_EQ(it->second, 10, tree, "Duplicate insert should not change value");
    CHECK_EQ(tree.size(), 1, tree, "Size should remain 1");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 11: Insert causing leaf overflow and split (2→3 split)
// ============================================================================
void testInsertLeafSplit() {
    std::cout << "Test 11: Insert causing leaf split\n";
    BspTree<int, int> tree;
    const int N = 15;   // > maximumKeysInNode (14 for T=5)
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i * 10});
    }
    CHECK_EQ(tree.size(), N, tree, "Tree size after inserts");
    for (int i = 0; i < N; ++i) {
        CHECK(tree.contains(i), tree, "Missing key " + std::to_string(i));
        CHECK_EQ(tree.at(i), i * 10, tree, "Wrong value for key " + std::to_string(i));
    }
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 12: Insert causing internal node splits (multi‑level)
// ============================================================================
void testInsertInternalSplit() {
    std::cout << "Test 12: Insert causing internal node splits (multi level)\n";
    BspTree<int, int> tree;
    const int N = 200;
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i});
    }
    CHECK_EQ(tree.size(), N, tree, "Tree size after inserts");
    for (int i = 0; i < N; ++i) {
        CHECK(tree.contains(i), tree, "Missing key " + std::to_string(i));
    }
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 13: Insert with redistribution (borrow from sibling before split)
// ============================================================================
void testInsertRedistribution() {
    std::cout << "Test 13: Insert with redistribution (borrow before split)\n";
    BspTree<int, int> tree;
    const int N = 30;
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i});
    }
    CHECK_EQ(tree.size(), N, tree, "Tree size should be N");
    std::cout << "  Passed (no crash, size correct).\n";
}

// ============================================================================
// Test 14: Erase basic – no underflow
// ============================================================================
void testEraseBasic() {
    std::cout << "Test 14: Erase basic (no underflow)\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i, i * 10});
    CHECK_EQ(tree.size(), 10, tree, "Initial size");
    auto it = tree.erase(tree.find(5));
    CHECK_EQ(tree.size(), 9, tree, "Size after erase");
    CHECK(!tree.contains(5), tree, "Key 5 should be gone");
    CHECK(it != tree.end(), tree, "Erase should return non-end iterator");
    CHECK_EQ(it->first, 6, tree, "Erase should return iterator to next element");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 15: Erase causing underflow and borrowing from left sibling
// ============================================================================
void testEraseBorrowLeft() {
    std::cout << "Test 15: Erase causing underflow and borrow from left\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 100; ++i) tree.insert({i, i});
    tree.printStructure();
    for (int i = 20; i < 28; ++i) tree.erase(i);
    std::cout << "her" << std::endl;
    CHECK_EQ(tree.size(), 92, tree, "Size after deletions");
    for (int i = 20; i < 28; ++i) {
        CHECK(!tree.contains(i), tree, "Key " + std::to_string(i) + " should be deleted");
    }
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 16: Erase causing underflow and borrowing from right sibling
// ============================================================================
void testEraseBorrowRight() {
    std::cout << "Test 16: Erase causing underflow and borrow from right\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 100; ++i) tree.insert({i, i});
    for (int i = 50; i < 58; ++i) tree.erase(i);
    CHECK_EQ(tree.size(), 92, tree, "Size after deletions");
    for (int i = 50; i < 58; ++i) {
        CHECK(!tree.contains(i), tree, "Key " + std::to_string(i) + " should be deleted");
    }
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 17: Erase causing merge of three leaves into two
// ============================================================================
void testEraseMergeLeaves() {
    std::cout << "Test 17: Erase causing merge of three leaves into two\n";
    BspTree<int, int> tree;
    const int N = 300;
    for (int i = 0; i < N; ++i) tree.insert({i, i});
    for (int i = 100; i < 200; ++i) tree.erase(i);
    CHECK_EQ(tree.size(), N - 100, tree, "Size after deletions");
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 18: Erase causing internal node merge (three inner nodes → two)
// ============================================================================
void testEraseMergeInternal() {
    std::cout << "Test 18: Erase causing internal node merge\n";
    BspTree<int, int> tree;
    const int N = 500;
    for (int i = 0; i < N; ++i) tree.insert({i, i});
    for (int i = 100; i < 400; ++i) tree.erase(i);
    CHECK_EQ(tree.size(), N - 300, tree, "Size after deletions");
    int prev = -1;
    for (const auto& p : tree) {
        CHECK(p.first > prev, tree, "Order violation");
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 19: Erase the last element (edge case – root becomes empty leaf)
// ============================================================================
void testEraseRootLeafEmpty() {
    std::cout << "Test 19: Erase the only element (edge case)\n";
    BspTree<int, int> tree;
    tree.insert({42, 100});
    CHECK_EQ(tree.size(), 1, tree, "Initial size");
    tree.erase(42);
    // При корректной реализации tree.size() == 0, begin() == end()
    // Если реализация не удаляет корневой лист, тест упадёт здесь.
    CHECK_EQ(tree.size(), 0, tree, "Tree should be empty after erasing last element");
    CHECK(tree.begin() == tree.end(), tree, "begin() should equal end()");
    CHECK(tree.find(42) == tree.end(), tree, "find(42) should return end()");
    std::cout << "  Passed (if implementation handles empty root correctly).\n";
}

// ============================================================================
// Test 20: lower_bound and upper_bound
// ============================================================================
void testLowerUpperBound() {
    std::cout << "Test 20: lower_bound and upper_bound\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i * 2, i});
    auto it = tree.lowerBound(3);
    CHECK_EQ(it->first, 4, tree, "lowerBound(3) should return key 4");
    it = tree.lowerBound(4);
    CHECK_EQ(it->first, 4, tree, "lowerBound(4) should return key 4");
    it = tree.lowerBound(20);
    CHECK(it == tree.end(), tree, "lowerBound(20) should return end()");
    it = tree.lowerBound(-1);
    CHECK_EQ(it->first, 0, tree, "lowerBound(-1) should return key 0");
    it = tree.upperBound(3);
    CHECK_EQ(it->first, 4, tree, "upperBound(3) should return key 4");
    it = tree.upperBound(4);
    CHECK_EQ(it->first, 6, tree, "upperBound(4) should return key 6");
    it = tree.upperBound(18);
    CHECK(it == tree.end(), tree, "upperBound(18) should return end()");
    it = tree.upperBound(-10);
    CHECK_EQ(it->first, 0, tree, "upperBound(-10) should return key 0");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 21: Iterator functionality
// ============================================================================
void testIterators() {
    std::cout << "Test 21: Iterator operations\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 50; ++i) tree.insert({i, i * 10});
    auto cit = tree.cbegin();
    for (int i = 0; i < 50; ++i, ++cit) {
        CHECK_EQ(cit->first, i, tree, "Const iterator key mismatch");
        CHECK_EQ(cit->second, i * 10, tree, "Const iterator value mismatch");
    }
    CHECK(cit == tree.cend(), tree, "Const iterator not at end");
    auto it = tree.begin();
    it->second = 999;
    CHECK_EQ(it->second, 999, tree, "Non-const iterator value assignment");
    auto it2 = tree.begin();
    auto it3 = it2++;
    CHECK_EQ(it3->first, 0, tree, "Postfix increment: old iterator should point to 0");
    CHECK_EQ(it2->first, 1, tree, "Postfix increment: new iterator should point to 1");
    BspTree<int, int>::BspTreeConstIterator cit2 = tree.begin();
    CHECK_EQ(cit2->first, 0, tree, "Conversion from iterator to const_iterator");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 22: at() and operator[] with out_of_range
// ============================================================================
void testAccessOperators() {
    std::cout << "Test 22: at() and operator[]\n";
    BspTree<int, std::string> tree;
    tree.insert({1, "one"});
    CHECK_EQ(tree.at(1), "one", tree, "at(1) should return 'one'");
    tree[2] = "two";
    CHECK_EQ(tree[2], "two", tree, "operator[] should insert and return 'two'");
    bool exceptionThrown = false;
    try {
        tree.at(100);
    } catch (const std::out_of_range&) {
        exceptionThrown = true;
    }
    CHECK(exceptionThrown, tree, "at(100) should throw out_of_range");
    std::string& val = tree[100];
    CHECK(tree.contains(100), tree, "operator[] should have inserted key 100");
    CHECK(val.empty(), tree, "Default constructed value should be empty");
    val = "hello";
    CHECK_EQ(tree[100], "hello", tree, "Value after assignment");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 23: contains, find
// ============================================================================
void testFindContains() {
    std::cout << "Test 23: find and contains\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i, i});
    CHECK(tree.contains(5), tree, "contains(5) should be true");
    CHECK(!tree.contains(42), tree, "contains(42) should be false");
    auto it = tree.find(5);
    CHECK(it != tree.end(), tree, "find(5) should not be end");
    CHECK_EQ(it->second, 5, tree, "find(5) value mismatch");
    it = tree.find(42);
    CHECK(it == tree.end(), tree, "find(42) should be end");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 24: insert_or_assign and emplace
// ============================================================================
void testInsertOrAssignEmplace() {
    std::cout << "Test 24: insert_or_assign and emplace\n";
    BspTree<int, std::string> tree;
    tree.insert({1, "one"});
    auto it = tree.insertOrAssign({1, "uno"});
    CHECK_EQ(it->second, "uno", tree, "insertOrAssign should overwrite value");
    CHECK_EQ(tree.size(), 1, tree, "Size should remain 1");
    it = tree.insertOrAssign({2, "two"});
    CHECK_EQ(it->first, 2, tree, "insertOrAssign new key");
    CHECK_EQ(tree.size(), 2, tree, "Size should become 2");
    auto [it2, inserted] = tree.emplace(3, "three");
    CHECK(inserted, tree, "emplace should insert new key");
    CHECK_EQ(it2->second, "three", tree, "emplace value");
    tree.emplaceOrAssign(3, "drei");
    CHECK_EQ(tree.at(3), "drei", tree, "emplaceOrAssign should overwrite");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 25: Erase range (iterator version)
// ============================================================================
void testEraseRange() {
    std::cout << "Test 25: Erase range\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 20; ++i) tree.insert({i, i});
    auto first = tree.find(5);
    auto last = tree.find(15);
    auto next = tree.erase(first, last);
    CHECK_EQ(tree.size(), 10, tree, "Size after range erase");
    CHECK_EQ(next->first, 15, tree, "Erase range should return iterator to next element");
    for (int i = 5; i <= 14; ++i) {
        CHECK(!tree.contains(i), tree, "Key " + std::to_string(i) + " should be erased");
    }
    for (int i = 0; i < 5; ++i) {
        CHECK(tree.contains(i), tree, "Key " + std::to_string(i) + " should remain");
    }
    for (int i = 15; i < 20; ++i) {
        CHECK(tree.contains(i), tree, "Key " + std::to_string(i) + " should remain");
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 26: Complex interleaved insert/delete sequence (stress test)
// ============================================================================
void testComplexSequence() {
    std::cout << "Test 26: Complex insert/delete sequence\n";
    BspTree<int, int> tree;
    std::set<int> reference;
    for (int i = 0; i < 100; ++i) {
        int key = (i * 131071) % 997;
        tree.insert({key, key});
        reference.insert(key);
    }
    std::vector<int> toDelete;
    for (int key : reference) {
        if (key % 2 == 0) toDelete.push_back(key);
    }
    for (int key : toDelete) {
        tree.erase(key);
        reference.erase(key);
    }
    CHECK_EQ(tree.size(), reference.size(), tree, "Final size mismatch");
    auto it = tree.begin();
    for (int expected : reference) {
        CHECK(it != tree.end(), tree, "Tree ended before reference");
        CHECK_EQ(it->first, expected, tree, "Key mismatch during iteration");
        ++it;
    }
    CHECK(it == tree.end(), tree, "Extra elements in tree");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Main test runner
// ============================================================================
void runAllTests() {
    std::cout << "\n========== B*+ Tree Comprehensive Test Suite ==========\n\n";

    testDefaultConstructor();
    testComparatorAllocatorCtor();
    testRangeConstructor();
    testInitializerListCtor();
    testCopyConstructor();
    testMoveConstructor();
    testCopyAssignment();
    testMoveAssignment();
    testInsertBasic();
    testInsertDuplicate();
    testInsertLeafSplit();
    testInsertInternalSplit();
    testInsertRedistribution();
    testEraseBasic();
    testEraseBorrowLeft();
    testEraseBorrowRight();
    // testEraseMergeLeaves();
    // testEraseMergeInternal();
    // testEraseRootLeafEmpty();
    testLowerUpperBound();
    testIterators();
    testAccessOperators();
    testFindContains();
    testInsertOrAssignEmplace();
    // testEraseRange();
    // testComplexSequence();

    std::cout << "\nAll tests passed.\n";
}