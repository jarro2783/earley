#include <iostream>
#include <vector>

#include "earley_hash_set.hpp"
#include "earley/timer.hpp"
#include "earley/stack.hpp"

namespace
{
  void
  print_time(size_t, const std::string&, const earley::Timer&);
}

int main(int, char**)
{
  constexpr size_t size = 10000000;
  earley::Stack<int> stack;
  earley::Timer stack_timer;

  for (size_t j = 0; j != 25; ++j)
  {
    stack.start();
    for (size_t i = 0; i != size/25; ++i)
    {
      stack.emplace_back();
    }
    stack.finalise();
  }

  print_time(size, "stack inserts", stack_timer);

  std::vector<int> vector;
  earley::Timer vector_timer;

  for (size_t i = 0; i != size; ++i)
  {
    vector.emplace_back(i);
  }

  print_time(size, "vector inserts", vector_timer);

  earley::Timer vector_lookup;

  size_t sum = 0;
  for (size_t i = 0; i != size; ++i)
  {
    sum += vector[i];
  }

  print_time(size, "vector lookups", vector_lookup);
  std::cout << sum << std::endl;

  earley::HashSet<int> hash;
  earley::Timer hash_timer;
  for (size_t i = 0; i != size; ++i)
  {
    hash.insert(i);
  }

  print_time(size, "hash set inserts", hash_timer);

  earley::Timer hash_find_timer;
  for (size_t i = 0; i != size; ++i)
  {
    hash.find(i);
  }

  print_time(size, "hash finds", hash_find_timer);

  return 0;
}

namespace
{
  void
  print_time
  (
    size_t size,
    const std::string& operation,
    const earley::Timer& timer
  )
  {
    std::cout << size << " " << operation << " took " <<
      timer.count<std::chrono::microseconds>() << " microseconds" << std::endl;
  }
}
