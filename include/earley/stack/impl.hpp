#ifndef EARLEY_STACK_IMP_HPP
#define EARLEY_STACK_IMP_HPP

namespace earley::detail
{
  template <typename T>
  struct stack_segment
  {
    stack_segment()
    : stack_segment(nullptr)
    {
    }

    stack_segment(const stack_segment* _previous)
    : stack_segment(_previous, 2000)
    {
    }

    stack_segment(const stack_segment* _previous, size_t size)
    : m_previous(_previous)
    , m_size(size)
    {
      m_memory = static_cast<T*>(operator new(size * sizeof(T)));
      m_top = m_memory;
      m_current = m_top;
    }

    ~stack_segment() {
      delete m_previous;

      for (auto i = m_memory; i != m_top; ++i)
      {
        i->~T();
      }

      delete m_memory;
    }

    T*
    top()
    {
      return m_top;
    }

    T*
    current()
    {
      return m_current;
    }

    size_t
    capacity() const
    {
      return m_size;
    }

    size_t
    size() const
    {
      return m_current - m_memory;
    }

    template <typename... Args>
    void
    emplace_back(Args&&... args)
    {
      new (m_current) T (std::forward<Args>(args)...);
      ++m_current;
    }

    void
    destroy_top()
    {
      for (auto i = m_top; i != m_current; ++i)
      {
        i->~T();
      }

      m_current = m_top;
    }

    void
    finalise()
    {
      m_top = m_current;
    }

    size_t
    top_size() const
    {
      return m_current - m_top;
    }

    private:

    const stack_segment* m_previous;
    T* m_memory;
    T* m_top;
    T* m_current;
    size_t m_size;
  };
}

#endif
