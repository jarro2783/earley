#ifndef EARLEY_GRAMMAR_UTIL_HPP_INCLUDED
#define EARLEY_GRAMMAR_UTIL_HPP_INCLUDED

#include <unordered_map>
#include <unordered_set>

#include "earley.hpp"

namespace earley
{
  enum NonTerminals {
    EPSILON = -1,
    END_OF_INPUT = -2,
  };

  typedef std::unordered_map<size_t, std::unordered_set<int>> FirstSet;
  typedef std::unordered_map<size_t, std::unordered_set<int>> FollowSet;

  template <typename Set>
  bool
  insert_scanner(const Scanner& scanner, Set& set)
  {
    if (scanner.right == -1)
    {
      return set.insert(scanner.left).second;
    }
    else
    {
      bool changed = false;
      auto left = scanner.left;

      while (left < scanner.right)
      {
        changed |= set.insert(left).second;
      }

      return changed;
    }
  }

  template <typename Iterator, typename Set>
  bool
  insert_range(Iterator begin, Iterator end, Set& set)
  {
    bool changed = false;

    while (begin != end)
    {
      changed |= set.insert(*begin).second;
      ++begin;
    }

    return changed;
  }

  std::unordered_map<size_t, std::unordered_set<int>>
  first_sets(const ParseGrammar& grammar);

  std::unordered_map<size_t, std::unordered_set<int>>
  follow_sets
  (
    const ParseGrammar& grammar,
    const std::unordered_map<size_t, std::unordered_set<int>>& firsts
  );

  template <typename Iterator,
    typename = std::enable_if<
      std::is_same<
        typename Iterator::value_type,
        Entry
      >::value
    >
  >
  std::unordered_set<size_t>
  first_set(Iterator begin, Iterator end,
    const std::unordered_map<size_t, std::unordered_set<int>>& firsts)
  {
    std::unordered_set<size_t> result;

    while (begin != end)
    {
      auto& entry = *begin;

      if (holds<Scanner>(entry))
      {
        insert_scanner(get<Scanner>(entry), result);
        break;
      }
      else
      {
        auto iter = firsts.find(get<size_t>(entry));
        if (iter != firsts.end())
        {
          insert_range(iter->second.begin(), iter->second.end(), result);

          if (iter->second.count(EPSILON) == 0)
          {
            break;
          }
        }
      }

      ++begin;
    }

    if (begin == end)
    {
      result.insert(EPSILON);
    }

    return result;
  }
}

#endif
