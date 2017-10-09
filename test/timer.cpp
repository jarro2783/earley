#include <iostream>
#include <vector>

#include "earley/timer.hpp"
#include "earley/stack.hpp"

int main(int argc, char** argv)
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

  std::cout << size << " stack inserts took " << 
    stack_timer.count<std::chrono::microseconds>() << " microseconds" << std::endl;

  std::vector<int> vector;
  earley::Timer vector_timer;

  for (size_t i = 0; i != size; ++i)
  {
    vector.emplace_back(i);
  }

  std::cout << size << " vector inserts took " <<
    vector_timer.count<std::chrono::microseconds>() << " microseconds" << std::endl;

  return 0;
}
