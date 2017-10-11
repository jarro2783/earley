#ifndef EARLEY_STACK_IMP_HPP
#define EARLEY_STACK_IMP_HPP

#include <cstring>
#include <type_traits>

namespace earley::detail
{
  template <typename T, bool trivial_destroy>
  struct stack_segment_destroy;

  template <typename T>
  struct stack_segment_destroy<T, true>
  {
    void
    operator()(T*, T*)
    {
    }
  };

  template <typename T, bool trivial_copy>
  struct stack_segment_copy;

  template <typename T>
  struct stack_segment_copy<T, true>
  {
    void
    operator()(T* begin, T* end, T* dest, T*& finish)
    {
      size_t size = end - begin;
      std::memmove(dest, begin, size * sizeof(T));
      finish = dest + size;
    }
  };

  template <typename T>
  struct stack_segment
  {
    private:
    using item_destroy = stack_segment_destroy<T,
      std::is_trivially_destructible<T>::value>;

    using item_copy = stack_segment_copy<T,
      std::is_trivially_copyable<T>::value>;

    item_destroy m_destroy;
    item_copy m_copy;

    public:
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

      m_destroy(m_memory, m_top);
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

    // There must be enough space already
    template <typename... Args>
    void
    append(T* begin, T* end)
    {
      m_copy(begin, end, m_top, m_current);
    }

    void
    destroy_top()
    {
      m_destroy(m_current, m_top);
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
