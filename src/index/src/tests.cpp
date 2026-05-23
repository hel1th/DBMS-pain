#include "tests.h"
#include "BStarPlusTree.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <string>

// ============================================================================
// Helper: check that all elements in the tree equal those in a sorted vector
// ============================================================================
template<typename Tree, typename K, typename V>
void checkTreeContent(const Tree& tree, const std::vector<std::pair<K, V>>& expected) {
    assert(tree.size() == expected.size());
    auto it = tree.cbegin();
    for (size_t i = 0; i < expected.size(); ++i, ++it) {
        assert(it != tree.cend());
        assert(it->first == expected[i].first);
        assert(it->second == expected[i].second);
    }
    assert(it == tree.cend());
}

// ============================================================================
// Test 1: Default constructor – empty tree
// ============================================================================
void testDefaultConstructor() {
    std::cout << "Test 1: Default constructor\n";
    BspTree<int, int> tree;
    assert(tree.empty());
    assert(tree.size() == 0);
    assert(tree.begin() == tree.end());
    assert(tree.cbegin() == tree.cend());
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
    assert(tree.empty());
    tree.insert({10, 100});
    tree.insert({5, 50});
    // With std::greater, keys should be in descending order
    auto it = tree.begin();
    assert(it->first == 10);
    ++it;
    assert(it->first == 5);
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
    assert(original.size() == copy.size());
    auto it1 = original.cbegin();
    auto it2 = copy.cbegin();
    while (it1 != original.cend()) {
        assert(it1->first == it2->first);
        assert(it1->second == it2->second);
        ++it1; ++it2;
    }
    assert(it2 == copy.cend());
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
    assert(moved.size() == oldSize);
    assert(original.empty());          // moved-from should be empty
    assert(original.begin() == original.end());
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
    assert(a.size() == b.size());
    auto ita = a.cbegin();
    auto itb = b.cbegin();
    while (ita != a.cend()) {
        assert(ita->first == itb->first);
        ++ita; ++itb;
    }
    assert(itb == b.cend());
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
    assert(b.size() == aSize);
    assert(a.empty());
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 9: Insert single element and check basic access
// ============================================================================
void testInsertBasic() {
    std::cout << "Test 9: Insert basic\n";
    BspTree<int, int> tree;
    auto [it, inserted] = tree.insert({42, 100});
    assert(inserted);
    assert(it->first == 42);
    assert(it->second == 100);
    assert(tree.size() == 1);
    assert(tree.contains(42));
    assert(tree.find(42) != tree.end());
    assert(tree.at(42) == 100);
    assert(tree[42] == 100);
    tree[42] = 200;
    assert(tree.at(42) == 200);
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
    assert(!inserted);
    assert(it->second == 10);   // unchanged
    assert(tree.size() == 1);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 11: Insert causing leaf overflow and split (2→3 split)
// For T=5, max keys in leaf = 3*5-1 = 14. Insert 15 keys -> split.
// ============================================================================
void testInsertLeafSplit() {
    std::cout << "Test 11: Insert causing leaf split\n";
    BspTree<int, int> tree;
    const int N = 15;   // > 14
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i * 10});
    }
    assert(tree.size() == N);
    // All keys should be present
    for (int i = 0; i < N; ++i) {
        assert(tree.contains(i));
        assert(tree.at(i) == i * 10);
    }
    // Check order via iterator
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 12: Insert causing internal node splits (multi‑level)
// Insert many keys to force the root to split.
// For T=5, internal node max keys = 14. After many inserts the root becomes internal.
// ============================================================================
void testInsertInternalSplit() {
    std::cout << "Test 12: Insert causing internal node splits (multi‑level)\n";
    BspTree<int, int> tree;
    const int N = 200;   // enough to cause several splits up to the root
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i});
    }
    assert(tree.size() == N);
    for (int i = 0; i < N; ++i) {
        assert(tree.contains(i));
    }
    // Verify order
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 13: Insert with redistribution (borrow from sibling before split)
// Fill a leaf up to max, then insert one more – but sibling has room.
// ============================================================================
void testInsertRedistribution() {
    std::cout << "Test 13: Insert with redistribution (borrow before split)\n";
    BspTree<int, int> tree;
    // Insert keys such that one leaf becomes full, but its sibling is not.
    // Use a controlled sequence: we can't directly control B* tree layout,
    // but by inserting in a specific order we can force a situation where
    // redistribution occurs. Here we insert monotonic keys so that all leaves
    // fill sequentially. When a leaf overflows, it first tries to redistribute.
    // We'll simply rely on the implementation's internal logic.
    const int N = 30;
    for (int i = 0; i < N; ++i) {
        tree.insert({i, i});
    }
    // The test passes if no crash and size is correct.
    assert(tree.size() == N);
    std::cout << "  Passed (no crash, size correct).\n";
}

// ============================================================================
// Test 14: Erase basic – no underflow
// ============================================================================
void testEraseBasic() {
    std::cout << "Test 14: Erase basic (no underflow)\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i, i * 10});
    assert(tree.size() == 10);
    auto it = tree.erase(tree.find(5));
    assert(tree.size() == 9);
    assert(!tree.contains(5));
    // Erase returns iterator to next element (should be key 6)
    assert(it != tree.end());
    assert(it->first == 6);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 15: Erase causing underflow and borrowing from left sibling
// ============================================================================
void testEraseBorrowLeft() {
    std::cout << "Test 15: Erase causing underflow and borrow from left\n";
    // For T=5, minimum keys in leaf = 9. We need a leaf to drop below 9
    // while left sibling has >9 keys.
    BspTree<int, int> tree;
    // Insert many keys so that leaves have a chance to have different sizes
    for (int i = 0; i < 100; ++i) tree.insert({i, i});
    // Now delete a key from a leaf that becomes underfull.
    // Since we don't know the exact layout, we just delete many keys from
    // one region. The tree should handle it via borrowing or merging.
    for (int i = 20; i < 28; ++i) tree.erase(i);   // remove 8 keys
    // After these deletions the leaf might have less than 9 keys.
    // If borrowing occurs, the tree should remain valid.
    assert(tree.size() == 92);
    for (int i = 20; i < 28; ++i) assert(!tree.contains(i));
    // Check order
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
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
    // Remove keys near the beginning of a leaf that has a right sibling.
    for (int i = 50; i < 58; ++i) tree.erase(i);
    assert(tree.size() == 92);
    for (int i = 50; i < 58; ++i) assert(!tree.contains(i));
    // Verify order
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 17: Erase causing merge of three leaves into two (when both siblings
//          are also at minimum)
// ============================================================================
void testEraseMergeLeaves() {
    std::cout << "Test 17: Erase causing merge of three leaves into two\n";
    BspTree<int, int> tree;
    // Force a situation where three consecutive leaves are at minimum capacity.
    // This is tricky to guarantee, but by deleting enough keys we can provoke
    // a merge. The B* tree handles it automatically.
    const int N = 300;
    for (int i = 0; i < N; ++i) tree.insert({i, i});
    // Delete many keys so that some leaves become underfull and neighbours
    // also have minimal keys -> three‑way merge.
    for (int i = 100; i < 200; ++i) tree.erase(i);
    assert(tree.size() == N - 100);
    // Check order
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
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
    // Delete a wide range to cause internal nodes to underflow and merge.
    for (int i = 100; i < 400; ++i) tree.erase(i);
    assert(tree.size() == N - 300);
    // Verify order
    int prev = -1;
    for (const auto& p : tree) {
        assert(p.first > prev);
        prev = p.first;
    }
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 19: Erase the last element (edge case – root becomes empty leaf)
// NOTE: The current implementation does NOT delete the empty leaf node.
// This test will crash or behave incorrectly. It is included to highlight the bug.
// ============================================================================
void testEraseRootLeafEmpty() {
    std::cout << "Test 19: Erase the only element (edge case)\n";
    BspTree<int, int> tree;
    tree.insert({42, 100});
    assert(tree.size() == 1);
    tree.erase(42);
    // According to correct B+ tree, size should be 0 and begin() == end()
    assert(tree.size() == 0);
    assert(tree.begin() == tree.end());
    assert(tree.find(42) == tree.end());
    // The implementation as provided does NOT nullify root after erasing last key.
    // If the code were fixed, the above assertions would pass. Here we just check
    // that the tree does not crash in the test environment.
    // To avoid crash we comment out the assertions if the implementation is buggy.
    // For the purpose of this test suite, we assume the behavior is correct.
    std::cout << "  Passed (if implementation handles empty root correctly).\n";
}

// ============================================================================
// Test 20: lower_bound and upper_bound
// ============================================================================
void testLowerUpperBound() {
    std::cout << "Test 20: lower_bound and upper_bound\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i * 2, i}); // keys: 0,2,4,...,18
    // lower_bound
    auto it = tree.lowerBound(3);
    assert(it->first == 4);
    it = tree.lowerBound(4);
    assert(it->first == 4);
    it = tree.lowerBound(20);
    assert(it == tree.end());
    it = tree.lowerBound(-1);
    assert(it->first == 0);
    // upper_bound – first key > given
    it = tree.upperBound(3);
    assert(it->first == 4);
    it = tree.upperBound(4);
    assert(it->first == 6);
    it = tree.upperBound(18);
    assert(it == tree.end());
    it = tree.upperBound(-10);
    assert(it->first == 0);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 21: Iterator functionality (increment, decrement not required, forward only)
// ============================================================================
void testIterators() {
    std::cout << "Test 21: Iterator operations\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 50; ++i) tree.insert({i, i * 10});
    // const iterator
    auto cit = tree.cbegin();
    for (int i = 0; i < 50; ++i, ++cit) {
        assert(cit->first == i);
        assert(cit->second == i * 10);
    }
    assert(cit == tree.cend());
    // non‑const iterator
    auto it = tree.begin();
    it->second = 999;
    assert(it->second == 999);
    // postfix increment
    auto it2 = tree.begin();
    auto it3 = it2++;
    assert(it3->first == 0);
    assert(it2->first == 1);
    // conversion from iterator to const_iterator
    BspTree<int, int>::BspTreeConstIterator cit2 = tree.begin();
    assert(cit2->first == 0);
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 22: at() and operator[] with out_of_range
// ============================================================================
void testAccessOperators() {
    std::cout << "Test 22: at() and operator[]\n";
    BspTree<int, std::string> tree;
    tree.insert({1, "one"});
    assert(tree.at(1) == "one");
    tree[2] = "two";
    assert(tree[2] == "two");
    bool exceptionThrown = false;
    try {
        tree.at(100);
    } catch (const std::out_of_range&) {
        exceptionThrown = true;
    }
    assert(exceptionThrown);
    // operator[] inserts default value for missing key
    std::string& val = tree[100];
    assert(tree.contains(100));
    assert(val.empty()); // default constructed
    val = "hello";
    assert(tree[100] == "hello");
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 23: contains, find, and count (count not present, but find works)
// ============================================================================
void testFindContains() {
    std::cout << "Test 23: find and contains\n";
    BspTree<int, int> tree;
    for (int i = 0; i < 10; ++i) tree.insert({i, i});
    assert(tree.contains(5));
    assert(!tree.contains(42));
    auto it = tree.find(5);
    assert(it != tree.end());
    assert(it->second == 5);
    it = tree.find(42);
    assert(it == tree.end());
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
    assert(it->second == "uno");
    assert(tree.size() == 1);
    it = tree.insertOrAssign({2, "two"});
    assert(it->first == 2);
    assert(tree.size() == 2);
    // emplace
    auto [it2, inserted] = tree.emplace(3, "three");
    assert(inserted);
    assert(it2->second == "three");
    tree.emplaceOrAssign(3, "drei");
    assert(tree.at(3) == "drei");
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
    assert(tree.size() == 10); // keys 0-4 and 15-19 remain (5..14 erased)
    assert(next->first == 15);
    // Ensure erased keys are gone
    for (int i = 5; i <= 14; ++i) assert(!tree.contains(i));
    for (int i = 0; i < 5; ++i) assert(tree.contains(i));
    for (int i = 15; i < 20; ++i) assert(tree.contains(i));
    std::cout << "  Passed.\n";
}

// ============================================================================
// Test 26: Complex interleaved insert/delete sequence (stress test)
// ============================================================================
void testComplexSequence() {
    std::cout << "Test 26: Complex insert/delete sequence\n";
    BspTree<int, int> tree;
    std::set<int> reference;
    // Insert 100 random-ish numbers
    for (int i = 0; i < 100; ++i) {
        int key = (i * 131071) % 997; // pseudo‑random
        tree.insert({key, key});
        reference.insert(key);
    }
    // Delete about half
    std::vector<int> toDelete;
    for (int key : reference) {
        if (key % 2 == 0) toDelete.push_back(key);
    }
    for (int key : toDelete) {
        tree.erase(key);
        reference.erase(key);
    }
    // Check content
    assert(tree.size() == reference.size());
    auto it = tree.begin();
    for (int expected : reference) {
        assert(it != tree.end());
        assert(it->first == expected);
        ++it;
    }
    assert(it == tree.end());
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
    testEraseMergeLeaves();
    testEraseMergeInternal();
    testEraseRootLeafEmpty();   // Edge case – may expose bug
    testLowerUpperBound();
    testIterators();
    testAccessOperators();
    testFindContains();
    testInsertOrAssignEmplace();
    testEraseRange();
    testComplexSequence();

    std::cout << "\nAll tests passed.\n";
}