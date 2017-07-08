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
      ++(*this);
    }

    HashSetIterator&
    operator++()
    {
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
    return operator==(lhs, rhs);
  }

  template <typename T>
  class HashSet
  {
    public:

    typedef HashSetIterator<T> const_iterator;
    typedef const_iterator iterator;

    HashSet()
    : m_occupied(false, m_size)
    , m_size(100)
    {
      m_memory = static_cast<T*>(malloc(m_size * sizeof(T)));
    }

    ~HashSet()
    {
      free(m_memory);
    }

    const_iterator
    begin() const
    {
      return const_iterator(this, 0);
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
      size_t key = std::hash<T>()(t) % m_size;
      size_t secondary = 1 + key % (m_size - 2);

      if (m_elements/3 > m_size/4)
      {
        resize();
      }

      while (true)
      {
        if (!m_occupied[key])
        {
          new(m_memory + key) T (t);
          m_occupied[key] = true;

          return std::make_pair(const_iterator(this, key), true);
        }
        else if (t == m_memory[key])
        {
          return std::make_pair(const_iterator(this, key), false);
        }
        else
        {
          key += secondary;
          if (key > m_size)
          {
            key -= m_size;
          }
        }
      }
    }

    const_iterator
    find(const T& t) const
    {
    }

    int
    count(const T& t) const
    {
      return find(t) != end() ? 1 : 0;
    }

    void
    resize()
    {
      //TODO make this exception safe
      m_size *= 2;
      m_occupied.resize(m_size, false);
      m_memory = static_cast<T*>(realloc(m_memory, m_size));
    }

    private:
    friend class HashSetIterator<T>;

    std::vector<bool> m_occupied;
    T* m_memory;
    size_t m_elements;
    size_t m_size;
  };
}
