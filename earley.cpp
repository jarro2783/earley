#include <deque>
#include <iostream>
#include <unordered_map>

#include "earley.hpp"

using namespace earley;

// Predict the next item sets for `which` set
//
// Preconditions:
//   len(item_sets) >= which + 1
//   all referenced nonterminals in item_sets and rules exist in dom(rules)
//   which < len(input)
void
process_set(
  ItemSetList& item_sets,
  const std::string& input,
  const std::unordered_map<size_t, ItemSet>& rules,
  size_t which
)
{
  std::deque<Item> to_process(item_sets[which].begin(), item_sets[which].end());

  while (!to_process.empty())
  {
    auto& current = to_process.front();

    auto pos = current.position();

    if (pos != current.end())
    {
      if (std::holds_alternative<Epsilon>(*pos))
      {
        // if it holds an epsilon advance
        item_sets[which].insert(current.next());
        to_process.push_back(current.next());
      } else if (std::holds_alternative<size_t>(*pos))
      {
        // if it holds a non-terminal, add entries that expect the
        // non terminal
        auto& nt = rules.find(std::get<size_t>(*pos))->second;
        for (auto& rule : nt)
        {
          if (item_sets[which].insert(rule).second)
          {
            to_process.push_back(rule.start(which));
          }
        }
      } else if (std::holds_alternative<char>(*pos)) {
        // we scan the terminal and add it to the next set if it matches
        // if input[which] == item then advance
        if (input[which] == std::get<char>(*pos))
        {
          item_sets[which+1].insert(current.next());
        }
      }
      else
      {
        std::cerr << "Item doesn't hold a value" << std::endl;
        return;
      }
    }
    else
    {
      // at the end of the item; a completion
      // we need to advance anything with our non-terminal to the right
      // of the current from where it was predicted into our current set
      auto ours = current.nonterminal();
      for (auto& item : item_sets[current.where()])
      {
        auto dot = item.position();
        if (dot != item.end() &&
            std::holds_alternative<size_t>(*dot) &&
            std::get<size_t>(*dot) == ours)
        {
          //bring it into our set
          auto next = item.next();
          if (item_sets[which].insert(next).second)
          {
            to_process.push_back(next);
          }
        }
      }
    }

    to_process.pop_front();
  }
}

void
process_input(
  size_t start,
  const std::string& input,
  const std::unordered_map<size_t, ItemSet>& rules
)
{
  ItemSetList item_sets(input.size() + 1);

  item_sets[0] = rules.find(start)->second;

  for (size_t i = 0; i != input.size(); ++i)
  {
    process_set(item_sets, input, rules, i);
  }

  // did we get to the end?
  bool parsed = false;
  for (auto& item : item_sets[input.size()])
  {
    if (item.position() == item.end() && item.where() == 0)
    {
      parsed = true;
      std::cout << "Parsed: " << input << std::endl;
    }
  }

  if (!parsed)
  {
    std::cout << "Unable to parse: " << input << std::endl;
  }

  size_t n = 0;
  for (auto& items : item_sets)
  {
    std::cout << "Position " << n << std::endl;
    for (auto& item : items)
    {
      std::cout << item << std::endl;
    }
    ++n;
  }
}

int main(int argc, char** argv)
{
  Item parens(0, {'(', 0ul, ')'});
  Item empty(0, {earley::Epsilon()});

  std::unordered_map<size_t, ItemSet> rules{
    {0ul, {parens, empty,}},
  };

  process_input(0, "()", rules);
  process_input(0, "(())", rules);
  process_input(0, "((", rules);

  // number -> | number [0-9]
  // sum -> number | sum + number
  std::unordered_map<size_t, ItemSet> numbers{
    {
      0,
      {
        {0, {Epsilon()}},
        {0, {0ul, '0'}},
        {0, {0ul, '1'}},
        {0, {0ul, '2'}},
        {0, {0ul, '3'}},
        {0, {0ul, '4'}},
        {0, {0ul, '5'}},
        {0, {0ul, '6'}},
        {0, {0ul, '7'}},
        {0, {0ul, '8'}},
        {0, {0ul, '9'}},
      },
    },
    {
      1,
      {
        {1, {0ul}},
        {1, {1ul, '+', 0ul}},
      },
    }
  };

  if (argc < 2)
  {
    std::cerr << "Give me an expression to parse" << std::endl;
    exit(1);
  }

  process_input(1, argv[1], numbers);

  return 0;
}
