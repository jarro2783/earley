#include <earley/stack/impl.hpp>

namespace earley
{
  template <typename T>
  class Stack
  {
    public:

    Stack();
    ~Stack();

    // Start a new contiguous sequence.
    T*
    start();

    // End the current contiguous sequence.
    void
    finalise();

    // Add something to the back of the stack.
    // Returns a pointer to the start of the current contiguous sequence.
    template <typename... Args>
    T*
    emplace_back(Args&&... args);

    // A pointer to the start of the current contiguous sequence.
    T*
    current();

    void
    destroy_top();

    private:
    detail::stack_segment<T>* m_top_segment;
    bool m_owned = false;
  };

  class StackOwned {};

  template <typename T>
  Stack<T>::Stack()
  {
    m_top_segment = new detail::stack_segment<T>;
  }

  template <typename T>
  Stack<T>::~Stack()
  {
    delete m_top_segment;
  }

  template <typename T>
  T*
  Stack<T>::start()
  {
    if (m_owned)
    {
      throw StackOwned();
    }

    m_owned = true;

    return m_top_segment->top();
  }

  template <typename T>
  template <typename... Args>
  T*
  Stack<T>::emplace_back(Args&&... args)
  {
    auto& top = *m_top_segment;
    if (top.size() == top.capacity())
    {
      // we need to reallocate and return the new pointer
      auto next = new detail::stack_segment<T>(m_top_segment, m_top_segment->size()*2);
      for (auto iter = top.top(); iter != top.current(); ++iter)
      {
        next->emplace_back(std::move(*iter));
      }

      top.destroy_top();

      m_top_segment = next;

      return next->top();
    }
    else
    {
      top.emplace_back(std::forward<Args>(args)...);
      return top.top();
    }
  }

  template <typename T>
  void
  Stack<T>::finalise()
  {
    m_owned = false;
    m_top_segment->finalise();
  }

  template <typename T>
  void
  Stack<T>::destroy_top()
  {
    m_top_segment->destroy_top();
  }
}
