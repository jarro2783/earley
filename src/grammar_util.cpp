#include "earley/grammar_util.hpp"

namespace earley
{

std::unordered_map<size_t, std::unordered_set<int>>
first_sets(const ParseGrammar& grammar)
{
  auto& rules = grammar.rules();
  std::unordered_map<size_t, std::unordered_set<int>> first_set;

  bool changed = true;
  while (changed)
  {
    changed = false;

    for (size_t i = 0; i != rules.size(); ++i)
    {
      auto& rule_list = rules[i];
      auto nt = i;
      auto& set = first_set[nt];

      for (auto& rule: rule_list)
      {
        if (rule.begin() == rule.end())
        {
          changed |= insert_value(-1, set);
          continue;
        }

        auto entry_iter = rule.begin();
        while (entry_iter != rule.end())
        {
          auto& entry = *entry_iter;
          if (holds<Scanner>(entry))
          {
            auto& scanner = get<Scanner>(entry);
            changed |= insert_scanner(scanner, set);
            break;
          }
          else
          {
            auto next = get<size_t>(entry);
            auto& next_first = first_set[next];

            for (auto first: next_first)
            {
              // skip epsilon for this set
              if (first == -1)
              {
                continue;
              }
              changed |= insert_value(first, set);
            }

            // bail if epsilon isn't in this item's first set
            if (next_first.count(-1) == 0)
            {
              break;
            }
          }
          ++entry_iter;
        }

        // add epsilon for this set if we got all the way to the end
        if (entry_iter == rule.end())
        {
          changed |= insert_value(-1, set);
        }
      }
    }
  }

  return first_set;
}

std::unordered_map<size_t, std::unordered_set<int>>
follow_sets
(
  const ParseGrammar& grammar,
  const std::unordered_map<size_t, std::unordered_set<int>>& firsts
)
{
  std::unordered_map<size_t, std::unordered_set<int>> follows;
  auto& rules = grammar.rules();

  follows[grammar.start()].insert(END_OF_INPUT);

  bool changed = true;
  while (changed)
  {
    changed = false;

    for (size_t i = 0; i != rules.size(); ++i)
    {
      auto lhs = i;
      auto& rule_list = rules[lhs];

      for (auto& rule: rule_list)
      {
        auto position = rule.begin();
        while (position != rule.end())
        {
          if (holds<size_t>(*position))
          {
            auto nt = get<size_t>(*position);
            auto first = first_set(position+1, rule.end(), firsts);

            if (first.count(EPSILON))
            {
              changed |= insert_range(follows[lhs].begin(),
                follows[lhs].end(),
                follows[nt]);

              first.erase(EPSILON);
            }

            changed |= insert_range(first.begin(), first.end(), follows[nt]);
          }

          ++position;
        }
      }
    }
  }

  return follows;
}

}
