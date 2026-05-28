#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <initializer_list>
#include <pp_allocator.h>
#include <associativeContainer.h>


#ifndef SYS_PROG_BS_PLUS_TREE_H
#define SYS_PROG_BS_PLUS_TREE_H

template <typename TKey, typename TValue, comparator<TKey> Compare = std::less<TKey>, std::size_t T = 5>
class BspTree final : private Compare
{
public:
    using TreeDataType = std::pair<TKey, TValue>;
    using TreeDataTypeConst = std::pair<const TKey, TValue>;
    using ValueType = TreeDataTypeConst;

private:
    static constexpr const size_t minimumKeysInNode = 2 * T - 1;
    static constexpr const size_t maximumKeysInRoot = 4 * T - 1;
    static constexpr const size_t maximumKeysInNode = 3 * T - 1;

    inline bool compareKeys(const TKey& lhs, const TKey& rhs) const;
    inline bool comparePairs(const TreeDataType& lhs, const TreeDataType& rhs) const;

    struct BspNodeBase
    {
        bool isTerminated;
        BspNodeBase() noexcept;
        virtual ~BspNodeBase() = default;
    };

    struct BspNodeTerm : public BspNodeBase
    {
        BspNodeTerm* next_;
        boost::container::static_vector<TreeDataType, maximumKeysInRoot + 1> data_;
        BspNodeTerm() noexcept;
    };

    struct BspNodeMiddle : public BspNodeBase
    {
        boost::container::static_vector<TKey, maximumKeysInRoot + 1> keys_;
        boost::container::static_vector<BspNodeBase*, maximumKeysInRoot + 2> pointers_;
        BspNodeMiddle() noexcept;
    };

    pp_allocator<ValueType> allocator_;
    BspNodeBase* root_;
    size_t size_;
    pp_allocator<ValueType> getAllocator() const noexcept;

public:
    // region constructors
    explicit BspTree(const Compare& cmp = Compare(), pp_allocator<ValueType> alloc = pp_allocator<ValueType>());
    explicit BspTree(pp_allocator<ValueType> alloc, const Compare& comp = Compare());

    template<input_iterator_for_pair<TKey, TValue> Iterator>
    explicit BspTree(Iterator begin, Iterator end, const Compare& cmp = Compare(), pp_allocator<ValueType> alloc = pp_allocator<ValueType>());

    BspTree(std::initializer_list<std::pair<TKey, TValue>> data, const Compare& cmp = Compare(), pp_allocator<ValueType> alloc = pp_allocator<ValueType>());
    // endregion

    // region five
    BspTree(const BspTree& other);
    BspTree(BspTree&& other) noexcept;
    BspTree& operator=(const BspTree& other);
    BspTree& operator=(BspTree&& other) noexcept;
    ~BspTree() noexcept;
    // endregion

    // region iterators
    class BspTreeIterator;
    class BspTreeConstIterator;

    class BspTreeIterator final
    {
        BspNodeTerm* node_;
        size_t index_;

    public:
        using value_type = TreeDataTypeConst;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = BspTreeIterator;

        friend class BspTree;
        friend class BspTreeConstIterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t currentNodeKeysCount() const noexcept;
        size_t index() const noexcept;

        explicit BspTreeIterator(BspNodeTerm* node = nullptr, size_t index = 0);
    };

    class BspTreeConstIterator final
    {
        const BspNodeTerm* node_;
        size_t index_;

    public:
        using value_type = TreeDataTypeConst;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = BspTreeConstIterator;

        friend class BspTree;
        friend class BspTreeIterator;

        BspTreeConstIterator(const BspTreeIterator& it) noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t currentNodeKeysCount() const noexcept;
        size_t index() const noexcept;

        explicit BspTreeConstIterator(const BspNodeTerm* node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    // endregion

    // region element access
    TValue& at(const TKey& key);
    const TValue& at(const TKey& key) const;
    TValue& operator[](const TKey& key);
    TValue& operator[](TKey&& key);

    void printIterator(const BspTreeIterator& it) const;
    void printStructure() const;
    // endregion

    // region iterator begins
    BspTreeIterator begin();
    BspTreeIterator end();
    BspTreeConstIterator begin() const;
    BspTreeConstIterator end() const;
    BspTreeConstIterator cbegin() const;
    BspTreeConstIterator cend() const;
    // endregion

    // region lookup
    size_t size() const noexcept;
    bool empty() const noexcept;
    BspTreeIterator find(const TKey& key);
    BspTreeConstIterator find(const TKey& key) const;
    BspTreeIterator lowerBound(const TKey& key);
    BspTreeConstIterator lowerBound(const TKey& key) const;
    BspTreeIterator upperBound(const TKey& key);
    BspTreeConstIterator upperBound(const TKey& key) const;
    bool contains(const TKey& key) const;
    // endregion

    // region modifiers
    void deleteSubtree(BspNodeBase* node);
    void clear() noexcept;

    void splitRoot();
    void splitRootInner();
    void handleLeafOverflow(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    void handleInnerOverflow(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node);
    bool tryRedistributeLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    bool tryRedistributeInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node);
    void splitLeaf2To3(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    void splitInner2To3(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node);

    std::pair<BspTreeIterator, bool> insert(const TreeDataType& data);
    std::pair<BspTreeIterator, bool> insert(TreeDataType&& data);
    template <typename... Args>
    std::pair<BspTreeIterator, bool> emplace(Args&&... args);

    BspTreeIterator insertOrAssign(const TreeDataType& data);
    BspTreeIterator insertOrAssign(TreeDataType&& data);
    template <typename... Args>
    BspTreeIterator emplaceOrAssign(Args&&... args);

    void handleLackOfKeysLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    void handleLackOfKeysInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* middle);
    bool tryBorrowLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    bool tryBorrowInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node);
    void mergeLeaf3To2(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf);
    void mergeInner3To2(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node);

    BspTreeIterator erase(BspTreeIterator pos);
    BspTreeIterator erase(BspTreeConstIterator pos);
    BspTreeIterator erase(BspTreeIterator beg, BspTreeIterator en);
    BspTreeIterator erase(BspTreeConstIterator beg, BspTreeConstIterator en);
    BspTreeIterator erase(const TKey& key);
    // endregion
};

// ----------------------------------------------------------------------
//                           Deduction guides
// ----------------------------------------------------------------------
template<std::input_iterator Iterator,
         comparator<typename std::iterator_traits<Iterator>::value_type::first_type> Compare = std::less<typename std::iterator_traits<Iterator>::value_type::first_type>,
         std::size_t T = 5, typename U>
BspTree(Iterator begin, Iterator end, const Compare& cmp = Compare(), pp_allocator<U> = pp_allocator<U>())
    -> BspTree<typename std::iterator_traits<Iterator>::value_type::first_type,
               typename std::iterator_traits<Iterator>::value_type::second_type, Compare, T>;

template<typename TKey, typename TValue, comparator<TKey> Compare = std::less<TKey>, std::size_t T = 5, typename U>
BspTree(std::initializer_list<std::pair<TKey, TValue>> data, const Compare& cmp = Compare(), pp_allocator<U> = pp_allocator<U>())
    -> BspTree<TKey, TValue, Compare, T>;

// ----------------------------------------------------------------------
//                         Implementation
// ----------------------------------------------------------------------
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::comparePairs(const TreeDataType& lhs, const TreeDataType& rhs) const
{
    return Compare::operator()(lhs.first, rhs.first);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::compareKeys(const TKey& lhs, const TKey& rhs) const
{
    return Compare::operator()(lhs, rhs);
}

// region BspNodeBase implementation
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspNodeBase::BspNodeBase() noexcept : isTerminated(false) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspNodeTerm::BspNodeTerm() noexcept : BspNodeBase(), next_(nullptr)
{
    this->isTerminated = true;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspNodeMiddle::BspNodeMiddle() noexcept {}
// endregion

// region constructors
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
pp_allocator<typename BspTree<TKey, TValue, Compare, T>::ValueType>
BspTree<TKey, TValue, Compare, T>::getAllocator() const noexcept
{
    return allocator_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::BspTreeConstIterator(const BspNodeTerm* node, size_t index)
    : node_(node), index_(index) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTree(const Compare& cmp, pp_allocator<ValueType> alloc)
    : Compare(cmp), allocator_(alloc), root_(nullptr), size_(0) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTree(pp_allocator<ValueType> alloc, const Compare& cmp)
    : BspTree(cmp, alloc) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
template<input_iterator_for_pair<TKey, TValue> Iterator>
BspTree<TKey, TValue, Compare, T>::BspTree(Iterator begin, Iterator end, const Compare& cmp, pp_allocator<ValueType> alloc)
    : Compare(cmp), allocator_(alloc), root_(nullptr), size_(0)
{
    for (auto it = begin; it != end; ++it)
        insert(*it);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTree(std::initializer_list<std::pair<TKey, TValue>> data,
                                           const Compare& cmp, pp_allocator<ValueType> alloc)
    : Compare(cmp), allocator_(alloc), root_(nullptr), size_(0)
{
    for (const auto& p : data)
        insert(p);
}
// endregion

// region copy and move
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTree(const BspTree& other)
    : Compare(other), allocator_(other.allocator_), root_(nullptr), size_(0)
{
    for (auto it = other.cbegin(); it != other.cend(); ++it)
        insert(*it);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTree(BspTree&& other) noexcept
    : Compare(std::move(other)), allocator_(std::move(other.allocator_)),
      root_(std::exchange(other.root_, nullptr)), size_(std::exchange(other.size_, 0)) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>& BspTree<TKey, TValue, Compare, T>::operator=(const BspTree& other)
{
    if (this != &other) {
        BspTree tmp(other);
        std::swap(root_, tmp.root_);
        std::swap(size_, tmp.size_);
        std::swap(allocator_, tmp.allocator_);
    }
    return *this;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>& BspTree<TKey, TValue, Compare, T>::operator=(BspTree&& other) noexcept
{
    if (this != &other) {
        clear();
        static_cast<Compare&>(*this) = std::move(static_cast<Compare&>(other));
        allocator_ = std::move(other.allocator_);
        root_ = other.root_;
        size_ = other.size_;
        other.root_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::~BspTree() noexcept
{
    clear();
}
// endregion

// region iterators
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTreeIterator::BspTreeIterator(BspNodeTerm* node, size_t index)
    : node_(node), index_(index) {}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator::reference
BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator*() const noexcept
{
    return reinterpret_cast<reference>(node_->data_[index_]);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator::pointer
BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator->() const noexcept
{
    return reinterpret_cast<pointer>(&node_->data_[index_]);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator&
BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator++()
{
    ++index_;
    if (index_ == node_->data_.size()) {
        if (node_->next_ == nullptr)
            *this = BspTreeIterator(nullptr, 0);
        else {
            node_ = node_->next_;
            index_ = 0;
        }
    }
    return *this;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator++(int)
{
    self temp = *this;
    ++*this;
    return temp;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator==(const self& other) const noexcept
{
    return node_ == other.node_ && index_ == other.index_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::BspTreeIterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
size_t BspTree<TKey, TValue, Compare, T>::BspTreeIterator::currentNodeKeysCount() const noexcept
{
    return node_->data_.size();
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
size_t BspTree<TKey, TValue, Compare, T>::BspTreeIterator::index() const noexcept
{
    return index_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::BspTreeConstIterator(const BspTreeIterator& it) noexcept
{
    index_ = it.index_;
    node_ = it.node_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::reference
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator*() const noexcept
{
    return reinterpret_cast<reference>(node_->data_[index_]);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::pointer
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator->() const noexcept
{
    return reinterpret_cast<pointer>(&node_->data_[index_]);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator&
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator++()
{
    ++index_;
    if (index_ == node_->data_.size()) {
        if (node_->next_ == nullptr)
            *this = BspTreeConstIterator(nullptr, 0);
        else {
            node_ = node_->next_;
            index_ = 0;
        }
    }
    return *this;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator
BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator++(int)
{
    self temp = *this;
    ++*this;
    return temp;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator==(const self& other) const noexcept
{
    return node_ == other.node_ && index_ == other.index_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
size_t BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::currentNodeKeysCount() const noexcept
{
    return node_->data_.size();
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
size_t BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator::index() const noexcept
{
    return index_;
}
// endregion

// region element access
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
TValue& BspTree<TKey, TValue, Compare, T>::at(const TKey& key)
{
    BspTreeIterator it = find(key);
    if (it == end())
        throw std::out_of_range("key not found");
    return it->second;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
const TValue& BspTree<TKey, TValue, Compare, T>::at(const TKey& key) const
{
    BspTreeConstIterator it = find(key);
    if (it == end())
        throw std::out_of_range("key not found");
    return it->second;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
TValue& BspTree<TKey, TValue, Compare, T>::operator[](const TKey& key)
{
    BspTreeIterator it = find(key);
    if (it == end()) {
        auto res = insert(TreeDataType(key, TValue()));
        it = res.first;
    }
    return it->second;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
TValue& BspTree<TKey, TValue, Compare, T>::operator[](TKey&& key)
{
    BspTreeIterator it = find(key);
    if (it == end()) {
        auto res = insert(TreeDataType(std::move(key), TValue()));
        it = res.first;
    }
    return it->second;
}
// endregion

// region iterator begins
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator BspTree<TKey, TValue, Compare, T>::begin()
{
    if (root_ == nullptr || size_ == 0)
        return end();
    BspNodeBase* node = root_;
    while (!node->isTerminated) {
        auto* middle = static_cast<BspNodeMiddle*>(node);
        node = middle->pointers_[0];
    }
    auto leaf = static_cast<BspNodeTerm*>(node);
    return BspTreeIterator(leaf, 0);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator BspTree<TKey, TValue, Compare, T>::end()
{
    return BspTreeIterator(nullptr, 0);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::begin() const
{
    if (root_ == nullptr || size_ == 0)
        return end();
    BspNodeBase* node = root_;
    while (!node->isTerminated) {
        auto* middle = static_cast<BspNodeMiddle*>(node);
        node = middle->pointers_[0];
    }
    auto leaf = static_cast<BspNodeTerm*>(node);
    return BspTreeConstIterator(leaf, 0);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::end() const
{
    return BspTreeConstIterator(nullptr, 0);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::cbegin() const
{
    if (root_ == nullptr || size_ == 0)
        return cend();
    BspNodeBase* node = root_;
    while (!node->isTerminated) {
        auto* middle = static_cast<BspNodeMiddle*>(node);
        node = middle->pointers_[0];
    }
    auto leaf = static_cast<BspNodeTerm*>(node);
    return BspTreeConstIterator(leaf, 0);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::cend() const
{
    return BspTreeConstIterator(nullptr, 0);
}
// endregion

// region lookup
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
size_t BspTree<TKey, TValue, Compare, T>::size() const noexcept
{
    return size_;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::empty() const noexcept
{
    return size_ == 0;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator BspTree<TKey, TValue, Compare, T>::find(const TKey& key)
{
    if (root_ == nullptr)
        return end();
    BspNodeBase* curr = root_;
    while (!curr->isTerminated) {
        auto* node = static_cast<BspNodeMiddle*>(curr);
        size_t i = 0;
        while (i < node->keys_.size() && !compareKeys(key, node->keys_[i]))
            ++i;
        curr = node->pointers_[i];
    }
    auto* nodeList = static_cast<BspNodeTerm*>(curr);
    int left = 0, right = nodeList->data_.size() - 1;
    while (left < right) {
        int mid = (left + right) / 2;
        bool less = compareKeys(key, nodeList->data_[mid].first);
        bool greater = compareKeys(nodeList->data_[mid].first, key);
        if (!greater && !less)
            return BspTreeIterator(nodeList, mid);
        if (less && !greater)
            right = mid - 1;
        else
            left = mid + 1;
    }
    if (!compareKeys(key, nodeList->data_[left].first) && !compareKeys(nodeList->data_[left].first, key))
        return BspTreeIterator(nodeList, left);
    return end();
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::find(const TKey& key) const
{
    return const_cast<BspTree*>(this)->find(key);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator BspTree<TKey, TValue, Compare, T>::lowerBound(const TKey& key)
{
    if (root_ == nullptr) return end();
    BspTreeIterator it = begin();
    while (it != end() && compareKeys(it->first, key))
        ++it;
    return it;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::lowerBound(const TKey& key) const
{
    if (root_ == nullptr) return end();
    BspTreeConstIterator it = begin();
    while (it != end() && compareKeys(it->first, key))
        ++it;
    return it;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator BspTree<TKey, TValue, Compare, T>::upperBound(const TKey& key)
{
    if (root_ == nullptr) return end();
    BspTreeIterator it = begin();
    while (it != end() && !compareKeys(key, it->first))
        ++it;
    return it;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeConstIterator BspTree<TKey, TValue, Compare, T>::upperBound(const TKey& key) const
{
    if (root_ == nullptr) return end();
    BspTreeConstIterator it = begin();
    while (it != end() && !compareKeys(key, it->first))
        ++it;
    return it;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::contains(const TKey& key) const
{
    return find(key) != end();
}
// endregion

// region modifiers
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::clear() noexcept
{
    deleteSubtree(root_);
    root_ = nullptr;
    size_ = 0;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::deleteSubtree(BspNodeBase* node)
{
    if (node == nullptr) return;
    if (node->isTerminated) {
        delete static_cast<BspNodeTerm*>(node);
    } else {
        auto* middle = static_cast<BspNodeMiddle*>(node);
        for (BspNodeBase* child : middle->pointers_)
            deleteSubtree(child);
        delete middle;
    }
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
std::pair<typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator, bool>
BspTree<TKey, TValue, Compare, T>::insert(const TreeDataType& data)
{
    return emplace(data);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
std::pair<typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator, bool>
BspTree<TKey, TValue, Compare, T>::insert(TreeDataType&& data)
{
    return emplace(std::move(data));
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::splitRoot()
{
    BspNodeMiddle* newRoot = new BspNodeMiddle();
    BspNodeTerm* rightChild = new BspNodeTerm();
    BspNodeTerm* leftChild = new BspNodeTerm();
    BspNodeTerm* oldRoot = static_cast<BspNodeTerm*>(root_);

    size_t newRootIndex = oldRoot->data_.size() / 2;
    TKey newRootValue = oldRoot->data_[newRootIndex].first;

    for (size_t i = 0; i < newRootIndex; ++i) {
        leftChild->data_.push_back(oldRoot->data_[i]);
    }
    for (size_t i = newRootIndex; i < oldRoot->data_.size(); ++i) {
        rightChild->data_.push_back(oldRoot->data_[i]);
    }

    leftChild->next_ = rightChild;
    rightChild->next_ = nullptr;

    newRoot->keys_.push_back(newRootValue);
    newRoot->pointers_.push_back(leftChild);
    newRoot->pointers_.push_back(rightChild);
    delete oldRoot;
    root_ = newRoot;
}


template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::splitRootInner()
{
    BspNodeMiddle* newRoot = new BspNodeMiddle();
    BspNodeMiddle* rightChild = new BspNodeMiddle();
    BspNodeMiddle* leftChild = new BspNodeMiddle();
    BspNodeMiddle* oldRoot = static_cast<BspNodeMiddle*>(root_);

    size_t newRootIndex = oldRoot->keys_.size() / 2;
    TKey newRootValue = oldRoot->keys_[newRootIndex];

    for (size_t i = 0; i < newRootIndex; ++i) {
        leftChild->keys_.push_back(oldRoot->keys_[i]);
    }
    for (size_t i = 0; i <= newRootIndex; ++i) {
        leftChild->pointers_.push_back(oldRoot->pointers_[i]);
    }

    for (size_t i = newRootIndex + 1; i < oldRoot->keys_.size(); ++i) {
        rightChild->keys_.push_back(oldRoot->keys_[i]);
    }
    for (size_t i = newRootIndex + 1; i < oldRoot->pointers_.size(); ++i) {
        rightChild->pointers_.push_back(oldRoot->pointers_[i]);
    }

    newRoot->keys_.push_back(newRootValue);
    newRoot->pointers_.push_back(leftChild);
    newRoot->pointers_.push_back(rightChild);
    delete oldRoot;
    root_ = newRoot;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::handleLeafOverflow(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{
    if (tryRedistributeLeaf(path, leaf)) {
        return;
    }
    splitLeaf2To3(path, leaf);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::handleInnerOverflow(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node)
{
    if (path.empty()) {
        splitRootInner();
        return;
    }
    if (tryRedistributeInner(path, node)) {
        return;
    }
    splitInner2To3(path, node);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::tryRedistributeLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;

    BspNodeTerm* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeTerm* leftBrother = (childIndex > 0) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex - 1]) : nullptr;

    if (rightBrother && rightBrother->data_.size() < maximumKeysInNode) {
        rightBrother->data_.insert(rightBrother->data_.begin(), leaf->data_.back());
        leaf->data_.pop_back();
        parent->keys_[childIndex] = rightBrother->data_[0].first;
        return true;
    }
    if (leftBrother && leftBrother->data_.size() < maximumKeysInNode) {
        leftBrother->data_.push_back(leaf->data_[0]);
        leaf->data_.erase(leaf->data_.begin());
        parent->keys_[childIndex - 1] = leaf->data_[0].first;
        return true;
    }
    return false;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::tryRedistributeInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node)
{
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;
    BspNodeMiddle* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeMiddle* leftBrother = (childIndex > 0) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex - 1]) : nullptr;

    if (rightBrother && rightBrother->keys_.size() < maximumKeysInNode) {
        rightBrother->pointers_.insert(rightBrother->pointers_.begin(), node->pointers_.back());
        node->pointers_.pop_back();

        rightBrother->keys_.insert(rightBrother->keys_.begin(), parent->keys_[childIndex]);
        parent->keys_[childIndex] = node->keys_.back();
        node->keys_.pop_back();
        return true;
    }
    if (leftBrother && leftBrother->keys_.size() < maximumKeysInNode) {
        leftBrother->pointers_.push_back(node->pointers_[0]);
        node->pointers_.erase(node->pointers_.begin());

        leftBrother->keys_.push_back(parent->keys_[childIndex - 1]);
        parent->keys_[childIndex - 1] = node->keys_[0];
        node->keys_.erase(node->keys_.begin());
        return true;
    }
    return false;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::splitLeaf2To3(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{

    BspNodeTerm* newLeaf = new BspNodeTerm();
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;
    path.pop();

    BspNodeTerm* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeTerm* leftBrother = (childIndex > 0) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex - 1]) : nullptr;
    size_t central_index = 0;

    std::vector<TreeDataType> merged;
    BspNodeTerm* leftNode = nullptr;
    BspNodeTerm* rightNode = nullptr;

    if (rightBrother == nullptr && leftBrother != nullptr) {
        leftNode = leftBrother;
        rightNode = leaf;
        central_index = childIndex; 
    } else if (rightBrother != nullptr) {
        leftNode = leaf;
        rightNode = rightBrother;
        central_index = childIndex + 1; 
    } else {
        return;
    }
    merged.insert(merged.end(), leftNode->data_.begin(), leftNode->data_.end());
    merged.insert(merged.end(), rightNode->data_.begin(), rightNode->data_.end());

    size_t overall = merged.size();
    size_t border1 = overall / 3;
    size_t border2 = 2 * overall / 3;

    leftNode->data_.clear();
    rightNode->data_.clear();

    for (size_t i = 0; i < border1; ++i) {
        leftNode->data_.push_back(merged[i]);
    }
    for (size_t i = border1; i < border2; ++i) {
        newLeaf->data_.push_back(merged[i]);
    }
    for (size_t i = border2; i < overall; ++i) {
        rightNode->data_.push_back(merged[i]);
    }

    BspNodeTerm* oldNext = rightNode->next_;
    leftNode->next_ = newLeaf;
    newLeaf->next_ = rightNode;
    rightNode->next_ = oldNext;

    parent->keys_.erase(parent->keys_.begin() + central_index - 1);
    parent->pointers_.erase(parent->pointers_.begin() + central_index);
    parent->keys_.insert(parent->keys_.begin() + central_index - 1, newLeaf->data_[0].first);
    parent->keys_.insert(parent->keys_.begin() + central_index, rightNode->data_[0].first);
    parent->pointers_.insert(parent->pointers_.begin() + central_index, newLeaf);
    parent->pointers_.insert(parent->pointers_.begin() + central_index + 1, rightNode);

    if (parent->keys_.size() > maximumKeysInNode) {
        handleInnerOverflow(path, parent);
    }
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::splitInner2To3(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node)
{
    BspNodeMiddle* newNode = new BspNodeMiddle();
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;
    path.pop();

    BspNodeMiddle* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeMiddle* leftBrother = (childIndex > 0) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex - 1]) : nullptr;
    size_t central_index = 0;

    BspNodeMiddle* leftNode = nullptr;
    BspNodeMiddle* rightNode = nullptr;

    if (rightBrother == nullptr && leftBrother != nullptr) {
        leftNode = leftBrother;
        rightNode = node;
        central_index = childIndex; 
    } else if (rightBrother != nullptr) {
        leftNode = node;
        rightNode = rightBrother;
        central_index = childIndex + 1;
    } else {
        delete newNode;
        return;
    }

    std::vector<TKey> mergedKeys;
    std::vector<BspNodeBase*> mergedPointers;
    

    mergedKeys.insert(mergedKeys.end(), leftNode->keys_.begin(), leftNode->keys_.end());
    mergedKeys.push_back(parent->keys_[central_index - 1]); // Спускаем разделитель
    mergedKeys.insert(mergedKeys.end(), rightNode->keys_.begin(), rightNode->keys_.end());

    mergedPointers.insert(mergedPointers.end(), leftNode->pointers_.begin(), leftNode->pointers_.end());
    mergedPointers.insert(mergedPointers.end(), rightNode->pointers_.begin(), rightNode->pointers_.end());

    leftNode->keys_.clear();
    leftNode->pointers_.clear();
    rightNode->keys_.clear();
    rightNode->pointers_.clear();

    size_t totalPointers = mergedPointers.size();
    size_t p1 = totalPointers / 3;
    size_t p2 = totalPointers / 3;
    size_t p3 = totalPointers - p1 - p2;

    size_t ptrIdx = 0;
    for (size_t i = 0; i < p1; ++i) {
        leftNode->pointers_.push_back(mergedPointers[ptrIdx++]);
    }
    for (size_t i = 0; i < p2; ++i) {
        newNode->pointers_.push_back(mergedPointers[ptrIdx++]);
    }
    for (size_t i = 0; i < p3; ++i) {
        rightNode->pointers_.push_back(mergedPointers[ptrIdx++]);
    } 

    size_t keyIndex = 0;
    
    for (size_t i = 0; i < p1 - 1; ++i) {
        leftNode->keys_.push_back(mergedKeys[keyIndex]);
        keyIndex++;
    }
    TKey parentKey1 = mergedKeys[keyIndex]; 
    ++keyIndex;

    for (size_t i = 0; i < p2 - 1; ++i) {
        newNode->keys_.push_back(mergedKeys[keyIndex]);
        ++keyIndex;
    }

    TKey parentKey2 = mergedKeys[keyIndex];
    ++keyIndex; 

    for (size_t i = 0; i < p3 - 1; ++i) {
        rightNode->keys_.push_back(mergedKeys[keyIndex]);
        ++keyIndex;
    }
    
    parent->keys_.erase(parent->keys_.begin() + central_index - 1);
    parent->pointers_.erase(parent->pointers_.begin() + central_index);

    parent->keys_.insert(parent->keys_.begin() + central_index - 1, parentKey1);
    parent->keys_.insert(parent->keys_.begin() + central_index, parentKey2);
    
    parent->pointers_.insert(parent->pointers_.begin() + central_index, newNode);
    parent->pointers_.insert(parent->pointers_.begin() + central_index + 1, rightNode);

    if (parent->keys_.size() > maximumKeysInNode) {
        handleInnerOverflow(path, parent);
    }
}


template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
template<typename... Args>
std::pair<typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator, bool>
BspTree<TKey, TValue, Compare, T>::emplace(Args&&... args)
{
    TreeDataType data(std::forward<Args>(args)...);
    std::stack<std::pair<BspNodeMiddle*, size_t>> path;
    if (root_ == nullptr) {
        auto* leaf = new BspNodeTerm();
        leaf->data_.push_back(data);
        root_ = leaf;
        ++size_;
        return {BspTreeIterator(leaf, 0), true};
    }
    if (root_->isTerminated) {
        BspNodeTerm* leaf = static_cast<BspNodeTerm*>(root_);
        size_t i = 0;
        while (i < leaf->data_.size() && compareKeys(leaf->data_[i].first, data.first)) {
            ++i;
        }
        if (i < leaf->data_.size() && !compareKeys(leaf->data_[i].first, data.first) && !compareKeys(data.first, leaf->data_[i].first)) {
            return {BspTreeIterator(leaf, i), false};
        }
        leaf->data_.insert(leaf->data_.begin() + i, data);
        ++size_;
        if (leaf->data_.size() > maximumKeysInRoot) {
            splitRoot();
            return {begin(), true};
        }
        return {BspTreeIterator(leaf, i), true};
    } else {
        BspNodeBase* curr = root_;
        while (!curr->isTerminated) {
            auto* middle = static_cast<BspNodeMiddle*>(curr);
            size_t i = 0;
            while (i < middle->keys_.size() && !compareKeys(data.first, middle->keys_[i])) {
                ++i;
            }
            path.push({middle, i});
            curr = middle->pointers_[i];
        }
        BspNodeTerm* leaf = static_cast<BspNodeTerm*>(curr);
        size_t i = 0;
        while (i < leaf->data_.size() && compareKeys(leaf->data_[i].first, data.first)) {
            ++i;
        }

        if (i < leaf->data_.size() && !compareKeys(leaf->data_[i].first, data.first) && !compareKeys(data.first, leaf->data_[i].first)) {
            return {BspTreeIterator(leaf, i), false};
        }
        leaf->data_.insert(leaf->data_.begin() + i, data);
        ++size_;
        if (leaf->data_.size() > maximumKeysInNode){
            handleLeafOverflow(path, leaf);
        }
        return {BspTreeIterator(leaf, i), true};
    }

}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::insertOrAssign(const TreeDataType& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    auto res = insert(data);
    return res.first;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::insertOrAssign(TreeDataType&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    auto res = insert(std::move(data));
    return res.first;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
template<typename... Args>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::emplaceOrAssign(Args&&... args)
{
    TreeDataType data(std::forward<Args>(args)...);
    return insertOrAssign(std::move(data));
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::erase(const TKey& key)
{
    std::stack<std::pair<BspNodeMiddle*, size_t>> path;
    if (find(key) == end()) return end();
    
    BspNodeBase* cur = root_;
    while (!cur->isTerminated) {
        auto* node = static_cast<BspNodeMiddle*>(cur);
        size_t i = 0;
        while (i < node->keys_.size() && !compareKeys(key, node->keys_[i])) {
            ++i;
        }
        path.push({node, i});
        cur = node->pointers_[i];
    }
    BspNodeTerm* nodeTerm = static_cast<BspNodeTerm*>(cur);
    size_t i = 0;
    while (i < nodeTerm->data_.size() && compareKeys(nodeTerm->data_[i].first, key)) {
        ++i;
    }
    size_t indexToRemove = i;
    TKey nextKey;
    bool hasNext = false;
    if (indexToRemove + 1 < nodeTerm->data_.size()) {
        nextKey = nodeTerm->data_[indexToRemove + 1].first;
        hasNext = true;
    } else if (nodeTerm->next_ != nullptr && !nodeTerm->next_->data_.empty()) {
        nextKey = nodeTerm->next_->data_[0].first;
        hasNext = true;
    }
    nodeTerm->data_.erase(nodeTerm->data_.begin() + indexToRemove);
    --size_;

    if (cur == root_) {
        if (nodeTerm->data_.size() == 0) {
            delete this->root_;
            root_ = nullptr;
        }
        return hasNext ? find(nextKey) : end();
    }
    
    if (nodeTerm->data_.size() < minimumKeysInNode) {
        if (!path.empty()) {
            handleLackOfKeysLeaf(path, nodeTerm);
        }
    }
    return hasNext ? find(nextKey) : end();
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::handleLackOfKeysLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{
    BspNodeMiddle * parent = path.top().first;
    if (parent->keys_.size() == 1) {
        BspNodeTerm * leftNode = static_cast<BspNodeTerm*>(parent->pointers_[0]);
        BspNodeTerm * rightNode = static_cast<BspNodeTerm*>(parent->pointers_[1]);

        leftNode->data_.insert(leftNode->data_.end(), rightNode->data_.begin(), rightNode->data_.end());
        leftNode->next_ = rightNode->next_;
        parent->keys_.clear();
        parent->pointers_.pop_back();
        delete rightNode;
        if (parent == this->root_) {
            this->root_ = leftNode;
            delete parent;
        }
        return;
    }
    if (tryBorrowLeaf(path, leaf)) {
        return;
    }
    mergeLeaf3To2(path, leaf);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::handleLackOfKeysInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* middle)
{
    if (path.empty()) {
        if (middle->keys_.size() == 0 && middle->pointers_.size() == 1) {
            BspNodeBase* child = middle->pointers_[0];
            delete middle;
            root_ = child;
        }
        return;
    }
    BspNodeMiddle * parent = path.top().first;
    if (parent->keys_.size() == 1) {
        BspNodeMiddle * leftNode = static_cast<BspNodeMiddle*>(parent->pointers_[0]);
        BspNodeMiddle * rightNode = static_cast<BspNodeMiddle*>(parent->pointers_[1]);

        leftNode->keys_.push_back(parent->keys_[0]);

        leftNode->keys_.insert(leftNode->keys_.end(), rightNode->keys_.begin(), rightNode->keys_.end());
        leftNode->pointers_.insert(leftNode->pointers_.end(), rightNode->pointers_.begin(), rightNode->pointers_.end());

        parent->keys_.clear();
        parent->pointers_.pop_back();
        delete rightNode;
        path.pop();
        handleLackOfKeysInner(path, parent);
        return;
    }
    if (tryBorrowInner(path, middle)) {
        return;
    }
    mergeInner3To2(path, middle);
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::tryBorrowLeaf(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{
    BspNodeTerm* rightBrother = nullptr;
    BspNodeTerm* leftBrother = nullptr;
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;

    if (childIndex < parent->keys_.size()) {
        rightBrother = static_cast<BspNodeTerm*>(parent->pointers_[childIndex + 1]);
        if (rightBrother && rightBrother->data_.size() > minimumKeysInNode) {
            leaf->data_.push_back(rightBrother->data_[0]);
            parent->keys_[childIndex] = rightBrother->data_[1].first;
            rightBrother->data_.erase(rightBrother->data_.begin());
            return true;
        }
    }

    if (childIndex > 0) {
        leftBrother = static_cast<BspNodeTerm*>(parent->pointers_[childIndex - 1]);
        if (leftBrother->data_.size() > minimumKeysInNode) {
            leaf->data_.insert(leaf->data_.begin(), leftBrother->data_.back());
            leftBrother->data_.pop_back();
            parent->keys_[childIndex - 1] = leaf->data_[0].first;
            return true;
        }
    }
    return false;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
bool BspTree<TKey, TValue, Compare, T>::tryBorrowInner(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* node)
{
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;

    if (childIndex < parent->keys_.size()) {
        BspNodeMiddle* rightBrother = static_cast<BspNodeMiddle*>(parent->pointers_[childIndex + 1]);
        if (rightBrother->keys_.size() > minimumKeysInNode) {
            node->pointers_.push_back(rightBrother->pointers_[0]);
            rightBrother->pointers_.erase(rightBrother->pointers_.begin());
            node->keys_.push_back(parent->keys_[childIndex]);
            parent->keys_[childIndex] = rightBrother->keys_[0];
            rightBrother->keys_.erase(rightBrother->keys_.begin());
            return true;
        }
    }
    if (childIndex > 0) {
        BspNodeMiddle* leftBrother = static_cast<BspNodeMiddle*>(parent->pointers_[childIndex - 1]);
        if (leftBrother->keys_.size() > minimumKeysInNode) {
            node->pointers_.insert(node->pointers_.begin(), leftBrother->pointers_.back());
            leftBrother->pointers_.pop_back();
            node->keys_.insert(node->keys_.begin(), parent->keys_[childIndex - 1]);
            parent->keys_[childIndex - 1] = leftBrother->keys_.back();
            leftBrother->keys_.pop_back();
            return true;
        }
    }
    return false;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::mergeLeaf3To2(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeTerm* leaf)
{
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;
    path.pop();

    std::vector<TreeDataType> merged;
    BspNodeTerm* leftNode = nullptr, *centralNode = nullptr, *rightNode = nullptr;

    BspNodeTerm* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeTerm* leftBrother = (childIndex > 0) ? static_cast<BspNodeTerm*>(parent->pointers_[childIndex - 1]) : nullptr;
    size_t centralIndex = 0;

    if (rightBrother == nullptr && leftBrother != nullptr) {
        rightNode = leaf;
        centralNode = leftBrother;
        leftNode = static_cast<BspNodeTerm*>(parent->pointers_[childIndex - 2]);
        centralIndex = childIndex - 1;
    } else if (rightBrother != nullptr && leftBrother == nullptr) {
        leftNode = leaf;
        centralNode = rightBrother;
        rightNode = static_cast<BspNodeTerm*>(parent->pointers_[childIndex + 2]);
        centralIndex = childIndex + 1;
    } else {
        rightNode = rightBrother;
        centralNode = leaf;
        leftNode = leftBrother;
        centralIndex = childIndex;
    }

    merged.insert(merged.end(), leftNode->data_.begin(), leftNode->data_.end());
    merged.insert(merged.end(), centralNode->data_.begin(), centralNode->data_.end());
    merged.insert(merged.end(), rightNode->data_.begin(), rightNode->data_.end());

    size_t overall = merged.size();
    size_t border = overall / 2;

    leftNode->data_.clear();
    rightNode->data_.clear();
    centralNode->data_.clear();

    for (size_t i = 0; i < border; ++i) leftNode->data_.push_back(merged[i]);
    for (size_t i = border; i < overall; ++i) rightNode->data_.push_back(merged[i]);

    BspNodeTerm* oldNext = rightNode->next_;
    leftNode->next_ = rightNode;
    rightNode->next_ = oldNext;
    delete centralNode;

    parent->keys_.erase(parent->keys_.begin() + (centralIndex - 1), parent->keys_.begin() + (centralIndex + 1));
    parent->pointers_.erase(parent->pointers_.begin() + centralIndex, parent->pointers_.begin() + centralIndex + 2);
    parent->keys_.insert(parent->keys_.begin() + (centralIndex - 1), rightNode->data_[0].first);
    parent->pointers_.insert(parent->pointers_.begin() + centralIndex, rightNode);
    if (parent->keys_.size() < minimumKeysInNode) {
        handleLackOfKeysInner(path, parent);
    }
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::mergeInner3To2(std::stack<std::pair<BspNodeMiddle*, size_t>>& path, BspNodeMiddle* middle)
{
    BspNodeMiddle* parent = path.top().first;
    size_t childIndex = path.top().second;
    path.pop();

    std::vector<TKey> mergedKeys;
    std::vector<BspNodeBase*> mergedPointers;
    BspNodeMiddle* leftNode = nullptr, *centralNode = nullptr, *rightNode = nullptr;

    BspNodeMiddle* rightBrother = (childIndex < parent->keys_.size()) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex + 1]) : nullptr;
    BspNodeMiddle* leftBrother = (childIndex > 0) ? static_cast<BspNodeMiddle*>(parent->pointers_[childIndex - 1]) : nullptr;
    size_t centralIndex = 0;

    if (rightBrother == nullptr && leftBrother != nullptr) {
        rightNode = middle;
        centralNode = leftBrother;
        leftNode = static_cast<BspNodeMiddle*>(parent->pointers_[childIndex - 2]);
        centralIndex = childIndex - 1;
    } else if (rightBrother != nullptr && leftBrother == nullptr) {
        leftNode = middle;
        centralNode = rightBrother;
        rightNode = static_cast<BspNodeMiddle*>(parent->pointers_[childIndex + 2]);
        centralIndex = childIndex + 1;
    } else {
        rightNode = rightBrother;
        centralNode = middle;
        leftNode = leftBrother;
        centralIndex = childIndex;
    }

    mergedKeys.insert(mergedKeys.end(), leftNode->keys_.begin(), leftNode->keys_.end());
    mergedKeys.push_back(parent->keys_[centralIndex - 1]);
    mergedKeys.insert(mergedKeys.end(), centralNode->keys_.begin(), centralNode->keys_.end());
    mergedKeys.push_back(parent->keys_[centralIndex]);
    mergedKeys.insert(mergedKeys.end(), rightNode->keys_.begin(), rightNode->keys_.end());

    mergedPointers.insert(mergedPointers.end(), leftNode->pointers_.begin(), leftNode->pointers_.end());
    mergedPointers.insert(mergedPointers.end(), centralNode->pointers_.begin(), centralNode->pointers_.end());
    mergedPointers.insert(mergedPointers.end(), rightNode->pointers_.begin(), rightNode->pointers_.end());

    size_t overallPointers = mergedPointers.size();
    size_t p1 = overallPointers / 2;
    size_t p2 = overallPointers - p1;

    leftNode->keys_.clear();
    rightNode->keys_.clear();
    leftNode->pointers_.clear();
    rightNode->pointers_.clear();

    for (size_t i = 0; i < p1; ++i) {
        leftNode->pointers_.push_back(mergedPointers[i]);
    }
    for (size_t i = p1; i < overallPointers; ++i) {
        rightNode->pointers_.push_back(mergedPointers[i]);
    }
    size_t keyIndex = 0;
    for (size_t i = 0; i < p1 - 1; ++i) {
        leftNode->keys_.push_back(mergedKeys[keyIndex]);
        ++keyIndex;
    }
    TKey toParent = mergedKeys[keyIndex];
    ++keyIndex;

    for (size_t i = 0; i < p2 - 1; ++i) {
        rightNode->keys_.push_back(mergedKeys[keyIndex]);
        ++keyIndex;
    }

    delete centralNode;

    parent->keys_.erase(parent->keys_.begin() + (centralIndex - 1), parent->keys_.begin() + (centralIndex + 1));
    parent->pointers_.erase(parent->pointers_.begin() + centralIndex, parent->pointers_.begin() + centralIndex + 2);

    parent->keys_.insert(parent->keys_.begin() + (centralIndex - 1), toParent);
    parent->pointers_.insert(parent->pointers_.begin() + centralIndex, rightNode);

    if (parent->keys_.size() < minimumKeysInNode) {
        handleLackOfKeysInner(path, parent);
    }
}



template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::erase(BspTreeIterator pos)
{
    if (pos == end()) return end();
    return erase(pos.node_->data_[pos.index_].first);;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::erase(BspTreeConstIterator pos)
{
    if (pos == end()) return end();
    BspTreeIterator next(const_cast<BspNodeTerm*>(pos.node_), pos.index_);
    ++next;
    erase(pos.node_->data_[pos.index_].first);
    return next;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::erase(BspTreeIterator beg, BspTreeIterator en)
{
    if (beg == end()) return end();
    
    bool hasEnd = (en != end());
    TKey endKey;
    if (hasEnd) endKey = en->first;

    // Удаляем элементы, пока текущий ключ строго меньше конечного ключа
    while (beg != end() && (!hasEnd || compareKeys(beg->first, endKey))) {
        beg = erase(beg);
    }
    return beg;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
typename BspTree<TKey, TValue, Compare, T>::BspTreeIterator
BspTree<TKey, TValue, Compare, T>::erase(BspTreeConstIterator beg, BspTreeConstIterator en)
{
    if (beg == end()) return end();
    
    bool hasEnd = (en != end());
    TKey endKey;
    if (hasEnd) endKey = en->first;

    BspTreeIterator current(const_cast<BspNodeTerm*>(beg.node_), beg.index_);
    while (current != end() && (!hasEnd || compareKeys(current->first, endKey))) {
        current = erase(current);
    }
    return current;
}

// endregion

// region debug printing
template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::printIterator(const BspTreeIterator& it) const
{
    if (it.node_ == nullptr) {
        std::cerr << "Iterator: end()" << std::endl;
        return;
    }
    std::cerr << "Iterator: node=" << it.node_ << " index=" << it.index_;
    if (it.index_ < it.node_->data_.size())
        std::cerr << " key=" << it.node_->data_[it.index_].first
                  << " value=" << it.node_->data_[it.index_].second;
    std::cerr << std::endl;
}

template<typename TKey, typename TValue, comparator<TKey> Compare, std::size_t T>
void BspTree<TKey, TValue, Compare, T>::printStructure() const
{
    if (root_ == nullptr) {
        std::cout << "Empty tree" << std::endl;
        return;
    }
    std::cout << "Tree elements (" << size() << "):\n";
    for (auto it = begin(); it != end(); ++it)
        std::cout << "[" << it.index() << "] " << it->first << " -> " << it->second << std::endl;
}
// endregion

#include <string>
template class BspTree<int, std::string>;
template class BspTree<int, int>;

#endif