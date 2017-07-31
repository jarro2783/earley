#ifndef EARLEY_VARIANT_HPP_INCLUDED
#define EARLEY_VARIANT_HPP_INCLUDED

#include <variant>

namespace earley
{
  using std::get;
  
  template <typename... Q, typename... T>
  auto
  holds(T&&... t)
  {
    return std::holds_alternative<Q...>(std::forward<T>(t)...);
  }
}

#endif
