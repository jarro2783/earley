#ifndef EARLEY_GRAMMAR_UTIL_HPP_INCLUDED
#define EARLEY_GRAMMAR_UTIL_HPP_INCLUDED

#include <unordered_map>
#include <unordered_set>

#include "earley.hpp"

namespace earley
{
  std::unordered_map<size_t, std::unordered_set<int>>
  first_sets(const ParseGrammar& grammar);
}

#endif
