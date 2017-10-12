#ifndef EARLEY_HASH_SET_HPP_INCLUDED
#define EARLEY_HASH_SET_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <vector>

namespace earley
{
  template <typename Key, typename Value,
    typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>>
  class HashTable;

  template <typename T,
    typename Hash = std::hash<T>,
    typename Equal = std::equal_to<T>>
  using HashSet = HashTable<T, void, Hash, Equal>;

  template <typename T, typename M, typename H, typename E>
  class HashSetIterator
  {
    public:

    typedef const T& reference;
    typedef T value_type;
    typedef size_t difference_type;
    typedef T* pointer;
    typedef std::forward_iterator_tag iterator_category;

    HashSetIterator(const HashTable<T, M, H, E>* hs, size_t pos)
    : m_set(hs)
    , m_pos(pos)
    {
    }

    HashSetIterator&
    operator++()
    {
      ++m_pos;
      while (m_pos != m_set->m_size
        && !m_set->m_occupied[m_pos])
      {
        ++m_pos;
      }

      return *this;
    }

    const auto&
    operator*() const
    {
      return *(m_set->m_memory + m_pos);
    }

    // TODO: fix this so that it can be constant
    // whilst allowing non-key items to be changed
    auto
    operator->() const
    {
      return m_set->m_memory + m_pos;
    }

    const HashTable<T, M, H, E>* m_set;
    size_t m_pos;
  };

  template <typename T, typename M, typename H, typename E>
  bool
  operator==(const HashSetIterator<T, M, H, E>& lhs,
    const HashSetIterator<T, M, H, E>& rhs)
  {
    return lhs.m_set == rhs.m_set && lhs.m_pos == rhs.m_pos;
  }

  template <typename T, typename M, typename H, typename E>
  bool
  operator!=(const HashSetIterator<T, M, H, E>& lhs,
    const HashSetIterator<T, M, H, E>& rhs)
  {
    return !operator==(lhs, rhs);
  }

  namespace detail
  {
    inline
    size_t
    next_prime (size_t number)
    {
      size_t i;

      for (number = (number / 2) * 2 + 3;; number += 2)
      {
        for (i = 3; i * i <= number; i += 2)
        {
          size_t q = number / i;
          if (q * i == number)
          {
            break;
          }
        }
        if (i * i > number)
        {
          return number;
        }
      }
    }

    template <typename T>
    struct HashStorageConstruct
    {
      template <typename Value>
      T
      operator()(Value&& v)
      {
        return T(std::forward<Value>(v));
      }
    };

    template <typename K, typename V>
    struct HashStorageConstruct<std::pair<K, V>>
    {
      using Type = std::pair<K, V>;

      template <typename Value>
      Type
      operator()(Value&& v)
      {
        return Type(std::forward<V>(v), V());
      }

      template <typename A, typename B>
      Type
      operator()(A&& a, B&& b)
      {
        return Type(std::move(a), std::move(b));
      }
    };

    struct identity
    {
      template <typename T>
      constexpr
      decltype(auto)
      operator()(T&& t) const
      {
        return std::forward<T>(t);
      }
    };

    struct first
    {
      template <typename A, typename B>
      constexpr
      auto
      operator()(const std::pair<A, B>& p) const
      {
        return p.first;
      }
    };
  }

  template <typename T,
    typename Mapping = void,
    typename Hash,
    typename Equality
  >
  class HashTable
  {
    private:
    using IsSet = std::is_same<Mapping, void>;

    using Storage = std::conditional_t<std::is_same_v<Mapping, void>,
      T, std::pair<const T, Mapping>>;
    using StorageSequence = std::conditional_t<std::is_same_v<Mapping, void>,
      int, std::make_index_sequence<2>>;
    using construct_storage_t = detail::HashStorageConstruct<Storage>;
    construct_storage_t construct_storage;

    using Key = std::conditional_t<IsSet::value,
      detail::identity,
      detail::first
    >;
    Key M_key;

    public:

    typedef HashSetIterator<T, Mapping, Hash, Equality> const_iterator;
    typedef const_iterator iterator;

    mutable size_t collisions = 0;

    HashTable()
    : HashTable(0)
    {
    }

    HashTable(size_t size)
    : m_elements(0)
    , m_size(size == 0 ? 0 : detail::next_prime(size))
    , m_first(m_size)
    , m_memory(nullptr)
    {
      if (m_size > 0)
      {
        m_occupied.insert(m_occupied.end(), m_size, false);
        m_memory = static_cast<Storage*>(malloc(m_size * sizeof(Storage)));
      }
    }

    HashTable(const HashTable&) = delete;

    HashTable(HashTable&& rhs)
    : m_occupied(std::move(rhs.m_occupied))
    {
      m_memory = rhs.m_memory;
      m_first = rhs.m_first;
      m_size = rhs.m_size;
      m_elements = rhs.m_elements;

      rhs.m_memory = nullptr;
      rhs.m_size = 0;
      rhs.m_elements = 0;
      rhs.m_first = 0;
    }

    ~HashTable()
    {
      for (size_t i = 0; i != m_size; ++i)
      {
        if (m_occupied[i])
        {
          m_memory[i].~Storage();
        }
      }

      free(m_memory);
    }

    const_iterator
    begin() const
    {
      return const_iterator(this, m_first);
    }

    const_iterator
    end() const
    {
      return const_iterator(this, m_size);
    }

    template <typename Iterator>
    void
    insert(Iterator begin, Iterator end)
    {
      while (begin != end)
      {
        insert(*begin);
        ++begin;
      }
    }

    std::pair<const_iterator, bool>
    insert(const Storage& t)
    {
      return insert_internal(t);
    }

    std::pair<const_iterator, bool>
    insert(Storage&& t)
    {
      return insert_internal(std::move(t));
    }

    template <typename... Args>
    std::pair<const_iterator, bool>
    emplace(Args&&... args)
    {
      return insert_internal(std::forward<Args>(args)...);
    }

    size_t
    size() const
    {
      return m_elements;
    }

    size_t
    capacity() const
    {
      return m_size;
    }

    private:

    size_t
    find_position(const T& t) const
    {
      // TODO: work out a better increment
      size_t key = Hash()(t);
      size_t secondary = 1 + key % (m_size - 2);
      //size_t secondary = 2;
      key %= m_size;

      while (true)
      {
        if (!m_occupied[key] || Equality()(t, M_key(m_memory[key])))
        {
          return key;
        }
        else
        {
          key += secondary;
          if (key >= m_size)
          {
            key -= m_size;
          }
        }
        ++collisions;
      }
    }

    public:

    const_iterator
    find(const T& t) const
    {
      if (m_size > 0)
      {
        auto pos = find_position(t);

        if (m_occupied[pos])
        {
          return HashSetIterator<T, Mapping, Hash, Equality>(this, pos);
        }
      }

      return HashSetIterator<T, Mapping, Hash, Equality>(this, m_size);
    }

    int
    count(const T& t) const
    {
      return find(t) != end() ? 1 : 0;
    }

    private:

    template <typename Value>
    std::pair<const_iterator, bool>
    insert_internal(Value&& t)
    {
      if (m_elements/3 >= m_size/4)
      {
        resize();
      }

      return insert_unchecked(std::forward<Value>(t));
    }

    template <typename S>
    auto
    insert_unpack(S&& storage, int)
    {
      return insert_impl(std::forward<S>(storage));
    }

    template <typename S, size_t... I>
    auto
    insert_unpack(S&& storage, std::index_sequence<I...>)
    {
      return insert_impl(std::get<I>(std::forward<S>(storage))...);
    }

    auto
    insert_unchecked(Storage&& storage)
    {
      return insert_unpack(std::move(storage), StorageSequence());
    }

    auto
    insert_unchecked(const Storage& storage)
    {
      return insert_unpack(storage, StorageSequence());
    }

    template <typename... Args>
    auto
    insert_unchecked(Args&&... args)
    {
      return insert_impl(std::forward<Args>(args)...);
    }

    template <typename Value, typename... Args>
    std::pair<const_iterator, bool>
    insert_impl(Value&& t, Args&&... args)
    {
      bool inserted = false;

      auto pos = find_position(t);

      if (!m_occupied[pos])
      {
        new(m_memory + pos) Storage (
          construct_storage(std::forward<Value>(t), std::forward<Args>(args)...)
        );
        m_occupied[pos] = true;
        inserted = true;
        ++m_elements;

        if (pos < m_first)
        {
          m_first = pos;
        }
      }

      return std::make_pair(
        HashSetIterator<T, Mapping, Hash, Equality>(this, pos), inserted);
    }

    void
    resize()
    {
      HashTable<T, Mapping, Hash, Equality> moved(m_size * 2 + 1);

      for (size_t i = 0; i != m_size; ++i)
      {
        if (m_occupied[i])
        {
          moved.insert_unchecked(std::move(m_memory[i])
          );
        }
      }

      swap(moved);
    }

    void
    swap(HashTable<T, Mapping, Hash, Equality>& other)
    {
      std::swap(other.m_memory, m_memory);
      std::swap(other.m_elements, m_elements);
      std::swap(other.m_occupied, m_occupied);
      std::swap(other.m_first, m_first);
      std::swap(other.m_size, m_size);
    }

    friend class HashSetIterator<T, Mapping, Hash, Equality>;

    size_t m_elements;
    size_t m_size;
    size_t m_first;
    std::vector<bool> m_occupied;
    Storage* m_memory;
  };

  template <typename Key, typename Value, typename... Args>
  using HashMap = HashTable<Key, Value, Args...>;
}

#endif
