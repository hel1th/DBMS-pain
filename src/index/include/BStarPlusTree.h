#ifndef DBMS_PAIN_BSTARPLUSTREE_H
#define DBMS_PAIN_BSTARPLUSTREE_H

#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <pp_allocator.h>
#include <associativeContainer.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BStarPlusTree final : private compare {
public:

    using treeDataType = std::pair<tkey, tvalue>;
    using treeDataTypeConst = std::pair<const tkey, tvalue>;
    using valueType = treeDataTypeConst;

private:

    static constexpr const size_t minimumKeysInNode = 2 * t - 1;
    static constexpr const size_t maximumKeysInNode = 3 * t - 1;

    // region comparators declaration

    inline bool CompareKeys(const tkey& lhs, const tkey& rhs) const {
        return compare::operator()(lhs, rhs);
    }
    inline bool ComparePairs(const treeDataType& lhs, const treeDataType& rhs) const {
        return compare::operator()(lhs, rhs);
    }

    // endregion comparators declaration

    struct BSPNodeBase
    {
        bool _isTerminated;

        BSPNodeBase() noexcept : _isTerminated(false) {}
        virtual ~BSPNodeBase() =default;
    };

    struct BSPNodeTerm : public BSPNodeBase
    {
        BSPNodeTerm* _next;
        boost::container::static_vector<treeDataType, maximumKeysInNode + 1> _data;
        BSPNodeTerm() noexcept : BSPNodeBase(), _next(nullptr) {
            this->_isTerminated = true;
        };
    };

    struct BSPNodeMiddle : public BSPNodeBase
    {
        boost::container::static_vector<tkey, maximumKeysInNode + 1> _keys;
        boost::container::static_vector<BSPNodeBase*, maximumKeysInNode + 2> _pointers;
        BSPNodeMiddle() noexcept : BSPNodeBase() {}
    };

    pp_allocator<valueType> _allocator;
    BSPNodeBase* _root;
    size_t _size;

    pp_allocator<valueType> GetAllocator() const noexcept {
        return _allocator;
    } // мб она нам и не нада

public:

    // region constructors declaration

    explicit BStarPlusTree(const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>()) : compare(cmp),
    _allocator(), _root(nullptr), _size(0) {} // аллокатор!

    explicit BStarPlusTree(pp_allocator<valueType> alloc, const compare& cmp = compare()) : BStarPlusTree(cmp, std::move(alloc)) {}

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BStarPlusTree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>());

    BStarPlusTree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<valueType> = pp_allocator<valueType>());

    // endregion constructors declaration

    // region five declaration

    BStarPlusTree(const BStarPlusTree& other) {
        this->_root = other._root;
        this->_allocator = other._allocator;
        this->_size = other._size;
        // а дальше скопировать узлы?
    }

    BStarPlusTree(BStarPlusTree&& other) noexcept {
        this->_root = other._root;
        this->_allocator = other._allocator;
        this->_size = other._size;
        // а дальше переместить узлы?
    }

    BStarPlusTree& operator=(const BStarPlusTree& other);

    BStarPlusTree& operator=(BStarPlusTree&& other) noexcept;

    ~BStarPlusTree() noexcept {
        clear(); // и чето еще
    }

    // endregion five declaration

    // region iterators declaration

    class BSPIterator;
    class BSPConstIterator;

    class BSPIterator final {
        BSPNodeTerm* _node;
        size_t _index;

    public:
        using ValueType = treeDataTypeConst;
        using reference = ValueType&;
        using pointer = ValueType*;
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType = ptrdiff_t;
        using self = BSPIterator;

        friend class BStarPlusTree;
        friend class BSPConstIterator;

        reference operator*() const noexcept {
            return _node->_data[_index];
        }
        pointer operator->() const noexcept {
            return &_node->_data[_index];
        }

        self& operator++() {
            if (_index == _node->_data.size()) {
                _node = _node->_next;
                _index = 0;
            } else {
                ++_index;
            }
            return BSPIterator(_node, _index);
        }

        self operator++(int) {
            self temp = *this;
            ++*this;
            return temp;
        }

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

        explicit BSPIterator(BSPNodeTerm* node = nullptr, size_t index = 0) : _node(node), _index(index) {}
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

            BSPConstIterator(const BSPIterator& it) noexcept {
                this->_index = it._index;
                this->_node = it._node;
            }

            reference operator*() const noexcept {
                return _node->_data[_index];
            }
            pointer operator->() const noexcept {
                return &_node->_data[_index];
            }

            self& operator++() {
                if (_index == _node->_data.size()) {
                    _node = _node->_next;
                    _index = 0;
                } else {
                    ++_index;
                }
                return BSPConstIterator(_node, _index);
            }

            self operator++(int) {
                self temp = *this;
                ++*this;
                return temp;
            }

            bool operator==(const self& other) const noexcept {
                return (this->_node == other._node && this->_index == other._index);
            };

            bool operator!=(const self& other) const noexcept {
                return !(*this == other);
            }

            size_t CurrentNodeKeysCount() const noexcept {
                return _node->_data.size();
            }

            size_t index() const noexcept {
                return this->_index;
            }

            explicit BSPConstIterator(const BSPNodeTerm* node = nullptr, size_t index = 0) : BSPConstIterator(BSPIterator(node, index)) {}
        };

        friend class BSPIterator;
        friend class BSPConstIterator;

        // endregion iterators declaration

        // region element access declaration

        /*
         * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
         */
        tvalue& at(const tkey& key) {
            if (find(key) == end()) {
                throw std::out_of_range("key not found");
            }
            return *find(key);
        }
        const tvalue& at(const tkey& key) const {
            if (find(key) == end()) {
                throw std::out_of_range("key not found:");
            }
            return *find(key); // в чом прикол? почему они одинаковые? мб что-то поменять?
        }

        /*
         * If key not exists, makes default initialization of value
         */
        tvalue& operator[](const tkey& key) {
            if (find(key) == end()) {
                return insert(key); // тут несовпадение возвращаемых типов будет
            }
            return *find(key);
        }

        tvalue& operator[](tkey&& key) {
            if (find(key) == end()) {
                return insert(std::move(key));
            }
            return *find(key);
        }

        // endregion element access declaration (begin, end - DONE)
        // region iterator begins declaration

        BSPIterator begin() {
            if (_root == nullptr || _size == 0) {
                return end();
            }

            BSPNodeMiddle * node = this->_root;
            while (!node->_isTerminated) {
                node = node->_pointers[0];
            }
            node = static_cast<BSPNodeTerm*>(node);
            return BSPIterator(node->_keys, 0);
        }

        BSPIterator end() {
            return BSPIterator(nullptr, 0);
        };

        BSPConstIterator begin() const {
            if (_root == nullptr || _size == 0) {
                return end();
            }
            BSPNodeMiddle * node = this->_root;
            while (!node->_isTerminated) {
                node = node->_pointers[0];
            }
            node = static_cast<BSPNodeTerm*>(node);
            return BSPConstIterator(node->_keys, 0);
        }

        BSPConstIterator end() const {
            return BSPConstIterator(nullptr, 0);
        }

        BSPConstIterator cbegin() const { // а зач он если есть методы выше?
            if (_root == nullptr || _size == 0) {
                return end();
            }
            BSPNodeMiddle * node = this->_root;
            while (!node->_isTerminated) {
                node = node->_pointers[0];
            }
            node = static_cast<BSPNodeTerm*>(node);
            return BSPConstIterator(node->_keys, 0);
        }

        BSPConstIterator cend() const {
            return BSPConstIterator(nullptr, 0);
        }

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
        int comparer(const bool less, const bool greater) const {
            if (!greater && !less) {return 0;}
            if (greater && !less) {
                return 1;
            } else {
                return -1;
            }
        }

        BSPIterator find(const tkey& key) {
            if (this->_root == nullptr) {
                return end();
            }
            BSPNodeBase * curr = this->_root;
            while (!curr->_isTerminated) {
                auto * node = static_cast<BSPNodeMiddle*>(curr);
                size_t i = 0;
                int cmp = comparer(CompareKeys(key, node->_keys[i]), CompareKeys(node->_keys[i], key));
                while (i < node->_keys.size() && cmp != 0) {
                    ++i;
                }
                curr = node->_pointers[i];
            }

            auto * NodeList = static_cast<BSPNodeTerm*>(curr);
            // бинарный поиск нужного ключа в узле
            int left = 0;
            int right = NodeList->_data.size() - 1;

            while (left < right) {
                int mid = (left + right) / 2;
                int cmp = comparer(CompareKeys(key, NodeList->_keys[mid], CompareKeys(NodeList->_keys[mid], key)));
                if (cmp == 0) {
                    return BSPIterator(NodeList, mid);
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


        BSPIterator lower_bound(const tkey& key) {
            if (this->_root == nullptr) {};
            BSPIterator iter = begin();
            int cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            while (cmp < 0) {
                ++iter;
                cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            }
            return iter;
        }

        BSPConstIterator lower_bound(const tkey& key) const {
            if (this->_root == nullptr) {};
            BSPConstIterator iter = begin();
            int cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            while (cmp < 0) {
                ++iter;
                cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            }
            return  iter;
        }

        BSPIterator upper_bound(const tkey& key) {
            if (this->_root == nullptr) {};
            BSPIterator iter = begin();
            int cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            while (cmp <= 0) {
                ++iter;
                cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            }
            return iter;
        }

        BSPConstIterator upper_bound(const tkey& key) const {
            if (this->_root == nullptr) {};
            BSPConstIterator iter = begin();
            int cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            while (cmp <= 0) {
                ++iter;
                cmp = comparer(CompareKeys(key, iter._node->_keys), CompareKeys(iter._node->_keys, key));
            }
            return iter;
        }

        bool contains(const tkey& key) const {
            return find(key) == end();
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
    };




#endif //DBMS_PAIN_BSTARPLUSTREE_H