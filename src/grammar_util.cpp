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
          if (set.insert(-1).second)
          {
            changed = true;
          }
          continue;
        }

        auto entry_iter = rule.begin();
        while (entry_iter != rule.end())
        {
          auto& entry = *entry_iter;
          if (holds<Scanner>(entry))
          {
            auto& scanner = get<Scanner>(entry);
            auto left = scanner.left;
            if (scanner.right == -1)
            {
              if (set.insert(static_cast<int>(left)).second)
              {
                changed = true;
              }
            }
            else
            {
              // TODO: get rid of this dirty hack
              // it's too fragile because I enumerate this in several places
              while (left <= scanner.right)
              {
                if (set.insert(static_cast<int>(left)).second)
                {
                  changed = true;
                }
                ++left;
              }
            }

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
              if (set.insert(first).second)
              {
                changed = true;
              }
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
          if (set.insert(-1).second)
          {
            changed = true;
          }
        }
      }
    }
  }

  return first_set;
}

}