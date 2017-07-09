#include <cstddef>
#include <iterator>
#include <vector>

namespace earley
{
  template <typename T>
  class HashSet;

  template <typename T>
  class HashSetIterator
  {
    public:

    typedef const T& reference;
    typedef T value_type;
    typedef size_t difference_type;
    typedef T* pointer;
    typedef std::forward_iterator_tag iterator_category;

    HashSetIterator(const HashSet<T>* hs, size_t pos)
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

    const HashSet<T>* m_set;
    size_t m_pos;
  };

  template <typename T>
  bool
  operator==(const HashSetIterator<T>& lhs, const HashSetIterator<T>& rhs)
  {
    return lhs.m_set == rhs.m_set && lhs.m_pos == rhs.m_pos;
  }

  template <typename T>
  bool
  operator!=(const HashSetIterator<T>& lhs, const HashSetIterator<T>& rhs)
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

  template <typename T>
  class HashSet
  {
    public:

    typedef HashSetIterator<T> const_iterator;
    typedef const_iterator iterator;

    mutable size_t collisions = 0;

    // TODO: this is far from ideal
    static size_t all_collisions;

    HashSet()
    : HashSet(101)
    {
    }

    HashSet(size_t size)
    : m_elements(0)
    , m_size(detail::next_prime(size))
    , m_first(m_size)
    , m_occupied(m_size, false)
    {
      m_memory = static_cast<T*>(malloc(m_size * sizeof(T)));
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
      bool inserted = false;

      if (m_elements/3 > m_size/4)
      {
        resize();
      }

      auto pos = find_position(t);

      if (!m_occupied[pos])
      {
        new(m_memory + pos) T (t);
        m_occupied[pos] = true;
        inserted = true;
        ++m_elements;

        if (pos < m_first)
        {
          m_first = pos;
        }
      }

      return std::make_pair(HashSetIterator<T>(this, pos), inserted);
    }

    private:

    size_t
    find_position(const T& t) const
    {
      // TODO: work out a better increment
      size_t key = std::hash<T>()(t);
      size_t secondary = 1 + key % (m_size - 2);
      //size_t secondary = 2;
      key %= m_size;

      while (true)
      {
        if (!m_occupied[key] || t == m_memory[key])
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
        ++all_collisions;
      }
    }

    public:

    const_iterator
    find(const T& t) const
    {
      auto pos = find_position(t);

      if (m_occupied[pos])
      {
        return HashSetIterator<T>(this, pos);
      }
      else
      {
        return HashSetIterator<T>(this, m_size);
      }
    }

    int
    count(const T& t) const
    {
      return find(t) != end() ? 1 : 0;
    }

    private:

    void
    resize()
    {
      HashSet<T> moved(m_size * 2 + 1);

      for (size_t i = 0; i != m_size; ++i)
      {
        if (m_occupied[i])
        {
          moved.insert(m_memory[i]);
        }
      }

      swap(moved);
    }

    void
    swap(HashSet<T>& other)
    {
      std::swap(other.m_memory, m_memory);
      std::swap(other.m_elements, m_elements);
      std::swap(other.m_occupied, m_occupied);
      std::swap(other.m_first, m_first);
      std::swap(other.m_size, m_size);
    }

    friend class HashSetIterator<T>;

    size_t m_elements;
    size_t m_size;
    size_t m_first;
    std::vector<bool> m_occupied;
    T* m_memory;
  };
}
