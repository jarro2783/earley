#ifndef EARLEY_HASH_SET_HPP_INCLUDED
#define EARLEY_HASH_SET_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <vector>

namespace earley
{
  template <typename T, typename Hash, typename Equal>
  class HashSet;

  template <typename T, typename H, typename E>
  class HashSetIterator
  {
    public:

    typedef const T& reference;
    typedef T value_type;
    typedef size_t difference_type;
    typedef T* pointer;
    typedef std::forward_iterator_tag iterator_category;

    HashSetIterator(const HashSet<T, H, E>* hs, size_t pos)
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

    const T&
    operator*() const
    {
      return *(m_set->m_memory + m_pos);
    }

    T*
    operator->() const
    {
      return m_set->m_memory + m_pos;
    }

    const HashSet<T, H, E>* m_set;
    size_t m_pos;
  };

  template <typename T, typename H, typename E>
  bool
  operator==(const HashSetIterator<T, H, E>& lhs, const HashSetIterator<T, H, E>& rhs)
  {
    return lhs.m_set == rhs.m_set && lhs.m_pos == rhs.m_pos;
  }

  template <typename T, typename H, typename E>
  bool
  operator!=(const HashSetIterator<T, H, E>& lhs, const HashSetIterator<T, H, E>& rhs)
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
  }

  template <typename T,
    typename Hash = std::hash<T>,
    typename Equality = std::equal_to<T>>
  class HashSet
  {
    public:

    typedef HashSetIterator<T, Hash, Equality> const_iterator;
    typedef const_iterator iterator;

    mutable size_t collisions = 0;

    HashSet()
    : HashSet(0)
    {
    }

    HashSet(size_t size)
    : m_elements(0)
    , m_size(size == 0 ? 0 : detail::next_prime(size))
    , m_first(m_size)
    , m_memory(nullptr)
    {
      if (m_size > 0)
      {
        m_occupied.insert(m_occupied.end(), m_size, false);
        m_memory = static_cast<T*>(malloc(m_size * sizeof(T)));
      }
    }

    HashSet(const HashSet&) = delete;

    HashSet(HashSet&& rhs)
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

    ~HashSet()
    {
      for (size_t i = 0; i != m_size; ++i)
      {
        if (m_occupied[i])
        {
          m_memory[i].~T();
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
    insert(const T& t)
    {
      return insert_internal(t);
    }

    std::pair<const_iterator, bool>
    insert(T&& t)
    {
      return insert_internal(std::move(t));
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
        if (!m_occupied[key] || Equality()(t, m_memory[key]))
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
          return HashSetIterator<T, Hash, Equality>(this, pos);
        }
      }

      return HashSetIterator<T, Hash, Equality>(this, m_size);
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

    template <typename Value>
    std::pair<const_iterator, bool>
    insert_unchecked(Value&& t)
    {
      bool inserted = false;

      auto pos = find_position(t);

      if (!m_occupied[pos])
      {
        new(m_memory + pos) T (std::forward<Value>(t));
        m_occupied[pos] = true;
        inserted = true;
        ++m_elements;

        if (pos < m_first)
        {
          m_first = pos;
        }
      }

      return std::make_pair(HashSetIterator<T, Hash, Equality>(this, pos), inserted);
    }

    void
    resize()
    {
      HashSet<T, Hash, Equality> moved(m_size * 2 + 1);

      for (size_t i = 0; i != m_size; ++i)
      {
        if (m_occupied[i])
        {
          moved.insert_unchecked(std::move(m_memory[i]));
        }
      }

      swap(moved);
    }

    void
    swap(HashSet<T, Hash, Equality>& other)
    {
      std::swap(other.m_memory, m_memory);
      std::swap(other.m_elements, m_elements);
      std::swap(other.m_occupied, m_occupied);
      std::swap(other.m_first, m_first);
      std::swap(other.m_size, m_size);
    }

    friend class HashSetIterator<T, Hash, Equality>;

    size_t m_elements;
    size_t m_size;
    size_t m_first;
    std::vector<bool> m_occupied;
    T* m_memory;
  };
}

#endif
