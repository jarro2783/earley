#include "catch.hpp"
#include <earley/stack.hpp>

template <typename T>
using Stack = earley::Stack<T>;

TEST_CASE("Stack", "[stack]")
{
  // This test relies on the fact that a stack is initialised with
  // 2000 elements for the first segment
  Stack<int> s;
  s.start();

  CHECK(s.top_size() == 0);
  auto values = s.emplace_back(5);
  CHECK(*values == 5);
  CHECK(s.top_size() == 1);

  auto values_after = values;
  for (size_t i = 0; i != 1500; ++i)
  {
    values_after = s.emplace_back(i);
  }
  CHECK(s.top_size() == 1501);
  s.finalise();
  CHECK(s.top_size() == 0);

  CHECK(values == values_after);

  s.start();
  for (size_t i = 0; i != 600; ++i)
  {
    values_after = s.emplace_back(i);
  }

  CHECK(values != values_after);
  CHECK(*values_after == 0);
  CHECK(s.top_size() == 600);

  s.destroy_top();

  CHECK(s.top_size() == 0);

  for (size_t i = 0; i != 3; ++i)
  {
    values_after = s.emplace_back(i);
  }

  CHECK(s.top_size() == 3);
}
