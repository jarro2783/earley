#include "catch.hpp"
#include <earley/stack.hpp>

template <typename T>
using Stack = earley::Stack<T>;

TEST_CASE("Stack", "[stack]")
{
  Stack<int> s;
  s.start();
  auto values = s.emplace_back(5);
  CHECK(*values == 5);

  auto values_after = values;
  for (size_t i = 0; i != 1500; ++i)
  {
    values_after = s.emplace_back(i);
  }
  s.finalise();

  CHECK(values == values_after);

  s.start();
  for (size_t i = 0; i != 600; ++i)
  {
    values_after = s.emplace_back(i);
  }

  CHECK(values != values_after);
  CHECK(*values_after == 0);
}
