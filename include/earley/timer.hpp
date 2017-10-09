#ifndef EARLEY_TIMER_HPP
#define EARLEY_TIMER_HPP

#include <chrono>

namespace earley
{
  class Timer
  {
    public:

    Timer()
    : m_start(std::chrono::system_clock::now())
    {
    }

    auto
    now() const
    {
      return std::chrono::system_clock::now();
    }

    template <typename T>
    double
    count() const
    {
      return std::chrono::duration_cast<T>(
        (now() - m_start)).count();
    }

    private:
    std::chrono::time_point<std::chrono::system_clock> m_start;
  };
}

#endif
