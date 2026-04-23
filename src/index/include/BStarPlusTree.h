#ifndef DBMS_PAIN_BSTARPLUSTREE_H
#define DBMS_PAIN_BSTARPLUSTREE_H

#include <associativeContainer.h>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <pp_allocator.h>
#include <stack>
#include <utility>
#include <vector>


template <typename tkey, typename tvalue,
          comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BStarPlusTree final : private compare {
public:
  using tree_data_type = std::pair<tkey, tvalue>;
  using tree_data_type_const = std::pair<const tkey, tvalue>;
  using value_type = tree_data_type_const;

private:
  // TODO: Another restrictions
  static constexpr const size_t minimum_keys_in_node = 2 * t - 1;
  static constexpr const size_t maximum_keys_in_node = 3 * t - 1;

  // region comparators declaration

  inline bool compare_keys(const tkey &lhs, const tkey &rhs) const {
    return compare::operator()(lhs, rhs); // а так можно?
  }
  inline bool compare_pairs(const tree_data_type &lhs,
                            const tree_data_type &rhs) const {
    return compare::operator()(lhs, rhs);
  }

  // endregion comparators declaration

  struct bsptree_node_base {
    bool _is_terminated;

    bsptree_node_base() noexcept;
    virtual ~bsptree_node_base() = default;
  };

  struct bsptree_node_term : public bsptree_node_base {
    bsptree_node_term *_next;
    boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>
        _data;
    bsptree_node_term() noexcept;
  };

  struct bsptree_node_middle : public bsptree_node_base {
    boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
    boost::container::static_vector<bsptree_node_base *,
                                    maximum_keys_in_node + 2>
        _pointers;
    bsptree_node_middle() noexcept;
  };

  pp_allocator<value_type> _allocator;
  bsptree_node_base *_root;
  size_t _size;

  pp_allocator<value_type> get_allocator() const noexcept;

public:
  // region constructors declaration

  explicit BSP_tree(const compare &cmp = compare(),
                    pp_allocator<value_type> = pp_allocator<value_type>());

  explicit BSP_tree(pp_allocator<value_type> alloc,
                    const compare &comp = compare());

  template <input_iterator_for_pair<tkey, tvalue> iterator>
  explicit BSP_tree(iterator begin, iterator end,
                    const compare &cmp = compare(),
                    pp_allocator<value_type> = pp_allocator<value_type>());

  BSP_tree(std::initializer_list<std::pair<tkey, tvalue>> data,
           const compare &cmp = compare(),
           pp_allocator<value_type> = pp_allocator<value_type>());

  // endregion constructors declaration

  // region five declaration

  BSP_tree(const BSP_tree &other);

  BSP_tree(BSP_tree &&other) noexcept;

  BSP_tree &operator=(const BSP_tree &other);

  BSP_tree &operator=(BSP_tree &&other) noexcept;

  ~BSP_tree() noexcept;

  // endregion five declaration

  // region iterators declaration

  class bsptree_iterator;
  class bsptree_const_iterator;

  class bsptree_iterator final {
    bsptree_node_term *_node;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = value_type &;
    using pointer = value_type *;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = bsptree_iterator;

    friend class BSP_tree;
    friend class bsptree_const_iterator;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    bool operator==(const self &other) const noexcept {
      return this->_node == other._node && this->_index == other._index;
    }
    bool operator!=(const self &other) const noexcept {
      return !(*this == other);
    }

    size_t current_node_keys_count() const noexcept {
      return this->_node->_data.size();
    }
    size_t index() const noexcept { return this->_index; };

    explicit bsptree_iterator(bsptree_node_term *node = nullptr,
                              size_t index = 0);
  };

  class bsptree_const_iterator final {
    const bsptree_node_term *_node;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = bsptree_const_iterator;

    friend class BSP_tree;
    friend class bsptree_iterator;

    bsptree_const_iterator(const bsptree_iterator &it) noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    bool operator==(const self &other) const noexcept {
      return (this->_node == other._node && this->_index == other._index);
    };
    bool operator!=(const self &other) const noexcept {
      return !(*this == other);
    }

    size_t current_node_keys_count() const noexcept {
      return this->_node->_data.size(); // он меня не понял по-моему
    }

    size_t index() const noexcept { return this->_index; }

    explicit bsptree_const_iterator(const bsptree_node_term *node = nullptr,
                                    size_t index = 0);
  };

  friend class btree_iterator;
  friend class btree_const_iterator;

  // endregion iterators declaration

  // region element access declaration

  /*
   * Returns a reference to the mapped value of the element with specified key.
   * If no such element exists, an exception of type std::out_of_range is
   * thrown.
   */
  tvalue &at(const tkey &);
  const tvalue &at(const tkey &) const;

  /*
   * If key not exists, makes default initialization of value
   */
  tvalue &operator[](const tkey &key);
  tvalue &operator[](tkey &&key);

  // endregion element access declaration
  // region iterator begins declaration

  bsptree_iterator begin();
  bsptree_iterator end() { return bsptree_iterator(nullptr, 0); };

  bsptree_const_iterator begin() const;
  bsptree_const_iterator end() const;

  bsptree_const_iterator cbegin() const;
  bsptree_const_iterator cend() const;

  // endregion iterator begins declaration

  // region lookup declaration

  size_t size() const noexcept { return this->_size; }
  bool empty() const noexcept { return this->_size == 0; }

  /*
   * Returns end() if not exist
   */

  bsptree_iterator
  find(const tkey &key) { // тут что-то фундаментально не так, потому что вскод
                          // не предлагает мне ничо вставить! почему?
    if (this->_root == nullptr) {
      return end();
    }
    bsptree_node_base *curr = this->_root;
    while (!curr->_is_terminated) {
      bsptree_node_middle *node = static_cast<bsptree_node_middle *>(curr);
      size_t i = 0;
      while (i < node->_keys.size() && !compare_keys(key, node->_keys[i])) {
        ++i;
      }
      curr = node->_pointers[i];
    }

    bsptree_node_term *node_list = static_cast<bsptree_node_term *>(curr);
    // бинарный поиск нужного ключа в узле
    int left = 0;
    int right = node_list->_data.size() - 1;

    while (left < right) {
      size_t mid = (left + right) / 2;
      int cmp = compare_keys(node_list->_data[mid],
                             key); // возможно сравнение не в том порядке
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
  bsptree_const_iterator find(const tkey &key) const {
    return static_cast<bsptree_const_iterator>(find(key));
  } // то же самое, что и для штуки выше, но вернуть другой итератор

  bsptree_iterator lower_bound(const tkey &key);
  bsptree_const_iterator lower_bound(const tkey &key) const;

  bsptree_iterator upper_bound(const tkey &key);
  bsptree_const_iterator upper_bound(const tkey &key) const;

  bool contains(const tkey &key) const {
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
  std::pair<bsptree_iterator, bool>
  insert(const tree_data_type &data) { // тут ожидается страшная писанина
    ++_size;
  }
  std::pair<bsptree_iterator, bool> insert(tree_data_type &&data) { ++_size; }

  template <typename... Args>
  std::pair<bsptree_iterator, bool> emplace(Args &&...args);

  /*
   * Updates value if key exists, delegates to emplace.
   */
  bsptree_iterator insert_or_assign(const tree_data_type &data);
  bsptree_iterator insert_or_assign(tree_data_type &&data);

  template <typename... Args>
  bsptree_iterator emplace_or_assign(Args &&...args);

  /*
   * Return iterator to node next ro removed or end() if key not exists
   */
  bsptree_iterator erase(bsptree_iterator pos) { --this->_size; }
  bsptree_iterator erase(bsptree_const_iterator pos) { --this->_size; }

  bsptree_iterator erase(bsptree_iterator beg, bsptree_iterator en);
  bsptree_iterator erase(bsptree_const_iterator beg, bsptree_const_iterator en);

  bsptree_iterator erase(const tkey &key) { --this->_size; }

  // endregion modifiers declaration
};

#endif // DBMS_PAIN_BSTARPLUSTREE_H