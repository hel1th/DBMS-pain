#ifndef DBMS_PAIN_BSTARPLUSTREE_H
#define DBMS_PAIN_BSTARPLUSTREE_H

#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associativeContainer.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BStarPlusTree final : private compare
{
    public:

    using treeDataType = std::pair<tkey, tvalue>;
    using treeDataTypeConst = std::pair<const tkey, tvalue>;
    using valueType = treeDataTypeConst;

    private:

    static constexpr const size_t minimumKeysInNode = 2 * t - 1;
    static constexpr const size_t maximumKeysInNode = 3 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const {
        return compare::operator()(lhs, rhs); // а так можно?
    }
    inline bool compare_pairs(const treeDataType& lhs, const treeDataType& rhs) const {
        return compare::operator()(lhs, rhs);
    }

    // endregion comparators declaration

    struct BSPNodeBase
    {
        bool _is_terminated;

        BSPNodeBase() noexcept;
        virtual ~BSPNodeBase() =default;
    };

    struct BSPNodeTerm : public BSPNodeBase
    {
        BSPNodeTerm* _next;
        boost::container::static_vector<treeDataType, maximumKeysInNode + 1> _data;
        BSPNodeTerm() noexcept;
    };

    struct BSPNodeMiddle : public BSPNodeBase
    {
        boost::container::static_vector<tkey, maximumKeysInNode + 1> _keys;
        boost::container::static_vector<BSPNodeBase*, maximumKeysInNode + 2> _pointers;
        BSPNodeMiddle() noexcept;
    };

    pp_allocator<valueType> _allocator;
    BSPNodeBase* _root;
    size_t _size;

    pp_allocator<valueType> GetAllocator() const noexcept; // мб она нам и не нада

public:

    // region constructors declaration

    explicit BStarPlusTree(const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>());

    explicit BStarPlusTree(pp_allocator<valueType> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BStarPlusTree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>());

    BStarPlusTree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>());

    // endregion constructors declaration

    // region five declaration

    BStarPlusTree(const BStarPlusTree& other);

    BStarPlusTree(BStarPlusTree&& other) noexcept;

    BStarPlusTree& operator=(const BStarPlusTree& other);

    BStarPlusTree& operator=(BStarPlusTree&& other) noexcept;

    ~BStarPlusTree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class BSPIterator;
    class BSPConstIterator;

    class BSPIterator final
    {
        BSPNodeTerm* _node;
        size_t _index;

    public:
        using ValueType = treeDataTypeConst;
        using reference = ValueType&;
        using pointer = ValueType*;
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType = ptrdiff_t;
        using self = BSPIterator;

        friend class BSP_tree;
        friend class BSPConstIterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept {
            return this->_node == other._node && this->_index == other._index;
        }
        bool operator!=(const self& other) const noexcept {
            return !(*this == other);
        }

        size_t CurrentNodeKeysCount() const noexcept {
            return this->_node->_data.size();
        }
        size_t index() const noexcept {
            return this->_index;
        };

        explicit BSPIterator(BSPNodeTerm* node = nullptr, size_t index = 0);

    };

    class BSPConstIterator final
    {
        const BSPNodeTerm* _node;
        size_t _index;

    public:

        using ValueType = treeDataTypeConst;
        using reference = const ValueType&;
        using pointer = const ValueType*;
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType = ptrdiff_t;
        using self = BSPConstIterator;

        friend class BStarPlusTree;
        friend class BSPIterator;

        BSPConstIterator(const BSPIterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept {
            return (this->_node == other._node && this->_index == other._index);
        };
        bool operator!=(const self& other) const noexcept {
            return !(*this == other);
        }

        size_t CurrentNodeKeysCount() const noexcept {
            return this->_node->_data.size(); // он меня не понял по-моему
        }

        size_t index() const noexcept {
            return this->_index;
        }

        explicit BSPConstIterator(const BSPNodeTerm* node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    BSPIterator begin();
    BSPIterator end() {
        return BSPIterator(nullptr, 0);
    };

    BSPConstIterator begin() const;
    BSPConstIterator end() const;

    BSPConstIterator cbegin() const;
    BSPConstIterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept {
        return this->_size;
    }
    bool empty() const noexcept {
        return this->_size == 0;
    }

    /*
     * Returns end() if not exist
     */

    BSPIterator find(const tkey& key) { // тут что-то фундаментально не так, потому что вскод не предлагает мне ничо вставить! почему?
        if (this->_root == nullptr) {
            return end();
        }
        BSPNodeBase * curr = this->_root;
        while (!curr->_is_terminated) {
            BSPNodeMiddle * node = static_cast<BSPNodeMiddle*>(curr);
            size_t i = 0;
            while (i < node->_keys.size() && !compare_keys(key, node->_keys[i])) {
                ++i;
            }
            curr = node->_pointers[i];
        }

        BSPNodeTerm * node_list = static_cast<BSPNodeTerm*>(curr);
        // бинарный поиск нужного ключа в узле
        int left = 0;
        int right = node_list->_data.size() - 1;

        while (left < right) {
            size_t mid = (left + right) / 2;
            int cmp = compare_keys(node_list->_data[mid], key); // возможно сравнение не в том порядке
            if (cmp == 0) { 
                return bsptree_iterator(node_list, mid);
            }
            if (cmp < 0) {
                right = mid - 1;
            } else {
                left = mid + 1;
            }
        }
        return end(); // мб говно

    }
    BSPConstIterator find(const tkey& key) const {
        return static_cast<BSPConstIterator>(find(key));
    } // то же самое, что и для штуки выше, но вернуть другой итератор

    BSPIterator lower_bound(const tkey& key);
    BSPConstIterator lower_bound(const tkey& key) const;

    BSPIterator upper_bound(const tkey& key);
    BSPConstIterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const {
        return find(key); // поправить немного чето не так явно
    };

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */

    // TODO: отдельные функции merge split checkIfFull узлов
    std::pair<BSPIterator, bool> insert(const treeDataType& data) { // тут ожидается страшная писанина
        ++_size;
    }
    std::pair<BSPIterator, bool> insert(treeDataType&& data) {
        ++_size;
    }

    template <typename ...Args>
    std::pair<BSPIterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    BSPIterator insert_or_assign(const treeDataType& data);
    BSPIterator insert_or_assign(treeDataType&& data);

    template <typename ...Args>
    BSPIterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    BSPIterator erase(BSPIterator pos) {
        --this->_size;
    }
    BSPIterator erase(BSPConstIterator pos) {
        --this->_size;
    }

    BSPIterator erase(BSPIterator beg, BSPIterator en);
    BSPIterator erase(BSPConstIterator beg, BSPConstIterator en);


    BSPIterator erase(const tkey& key) {
        --this->_size;
    }

    // endregion modifiers declaration
};



#endif //DBMS_PAIN_BSTARPLUSTREE_H