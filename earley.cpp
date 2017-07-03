#include <chrono>
#include <deque>
#include <iostream>
#include <unordered_map>

#include "earley.hpp"

using namespace earley;

size_t
rule_id(
  std::unordered_map<std::string, size_t>& ids,
  size_t& next,
  const std::string& name
)
{
  auto iter = ids.find(name);

  if (iter == ids.end())
  {
    ids[name] = next;
    return next++;
  }
  else
  {
    return iter->second;
  }
}

struct CreateEntry
{
  CreateEntry(std::unordered_map<std::string, size_t>& ids, size_t& next_id)
  : m_ids(ids)
  , m_next(next_id)
  {
  }

  template <typename T>
  Entry
  operator()(T& t) const
  {
    return t;
  }

  Entry
  operator()(const std::string& s) const
  {
    return rule_id(m_ids, m_next, s);
  }

  private:

  std::unordered_map<std::string, size_t>& m_ids;
  size_t& m_next;
};

Entry
make_entry(
  const Production& production,
  std::unordered_map<std::string, size_t>& identifiers,
  size_t& next_id
)
{
  CreateEntry creator(identifiers, next_id);
  return std::visit(creator, production);
}

std::tuple<std::unordered_map<size_t, RuleList>,
  std::unordered_map<std::string, size_t>>
generate_rules(Grammar& grammar)
{
  size_t next_id = 0;
  std::unordered_map<std::string, size_t> identifiers;
  std::unordered_map<size_t, RuleList> rule_set;

  for (auto& nonterminal : grammar)
  {
    RuleList rules;
    auto id = rule_id(identifiers, next_id, nonterminal.first);

    for (auto& productions : nonterminal.second)
    {
      std::vector<Entry> entries;
      for (auto& production : productions)
      {
        entries.push_back(make_entry(production, identifiers, next_id));
      }
      Rule rule(id, entries);
      rules.push_back(rule);
    }

    rule_set[id] = std::move(rules);
  }

  return std::make_tuple(rule_set, identifiers);
}

// Predict the next item sets for `which` set
//
// Preconditions:
//   len(item_sets) >= which + 1
//   all referenced nonterminals in item_sets and rules exist in dom(rules)
//   which < len(input)
bool
process_set(
  ItemSetList& item_sets,
  const std::string& input,
  const std::unordered_map<size_t, earley::RuleList>& rules,
  size_t which
)
{
  bool more = false;
  std::deque<Item> to_process(item_sets[which].begin(), item_sets[which].end());

#if 0
  std::cout << "Processing " << which << " with item set:" << std::endl;
  for (auto& item : to_process)
  {
    std::cout << item << std::endl;
  }
#endif

  while (!to_process.empty())
  {
    auto& current = to_process.front();

    auto pos = current.position();

    if (pos != current.end())
    {
      if (std::holds_alternative<Epsilon>(*pos))
      {
        // if it holds an epsilon advance
        if (item_sets[which].insert(current.next()).second)
        {
          //std::cout << "Epsilon" << std::endl;
          to_process.push_back(current.next());
          more = true;
        }
      } else if (std::holds_alternative<size_t>(*pos))
      {
        // Predict
        // if it holds a non-terminal, add entries that expect the
        // non terminal
        auto& nt = rules.find(std::get<size_t>(*pos))->second;
        for (auto& rule : nt)
        {
          auto predict = Item(rule, which);
          if (item_sets[which].insert(predict).second)
          {
            //std::cout << "Predict " << std::get<size_t>(*pos) << std::endl;
            to_process.push_back(predict);
            more = true;
          }
        }
      }
      else if (std::holds_alternative<char>(*pos))
      {
        // Scan
        // we scan the terminal and add it to the next set if it matches
        // if input[which] == item then advance
        if (which < input.length() && input[which] == std::get<char>(*pos))
        {
          item_sets[which+1].insert(current.next());
        }
      }
      else
      {
        std::cerr << "Item doesn't hold a value" << std::endl;
        return false;
      }
    }
    else
    {
      // Complete
      // at the end of the item; a completion
      // we need to advance anything with our non-terminal to the right
      // of the current from where it was predicted into our current set
      auto ours = current.nonterminal();
      //std::cout << "Loop " << which << " completed " << ours << std::endl;
      for (auto& item : item_sets[current.where()])
      {
        //std::cout << "Consider " << item << std::endl;
        auto dot = item.position();
        if (dot != item.end() &&
            std::holds_alternative<size_t>(*dot) &&
            std::get<size_t>(*dot) == ours)
        {
          //bring it into our set
          auto next = item.next();
          if (item_sets[which].insert(next).second)
          {
            //std::cout << "Bring it into our set as " << next << std::endl;
            to_process.push_back(next);
            more = true;
          }
        }
      }
    }

    to_process.pop_front();
  }

  return more;
}

std::tuple<bool, double>
process_input(
  bool debug,
  size_t start,
  const std::string& input,
  const std::unordered_map<size_t, RuleList>& rules
)
{
  ItemSetList item_sets(input.size() + 1);

  auto& first_set = item_sets[0];

  for (auto& rule: rules.find(start)->second)
  {
    first_set.insert(Item(rule));
  }

  std::chrono::time_point<std::chrono::system_clock> start_time, end;
  start_time = std::chrono::system_clock::now();

  for (size_t i = 0; i != input.size() + 1; ++i)
  {
    while (process_set(item_sets, input, rules, i));
  }

  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start_time;

  if (debug)
  {
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

  // did we get to the end?
  bool parsed = false;
  for (auto& item : item_sets[input.size()])
  {
    if (item.position() == item.end() && item.where() == 0 && item.nonterminal() == start)
    {
      parsed = true;
      if (debug)
      {
        std::cout << "Parsed: " << input << std::endl;
        std::cout << item << std::endl;
      }
    }
  }

  //std::cout << "parsing took: " << elapsed_seconds.count() << "s\n";

  return std::make_tuple(parsed,
    std::chrono::duration_cast<std::chrono::microseconds>(elapsed_seconds).count());
}

struct InvertVisitor
{
  InvertVisitor(const std::unordered_map<size_t, earley::RuleList>& rules,
    std::vector<std::vector<const Rule*>>& inverted,
    const Rule* current
  )
  : m_rules(rules)
  , m_inverted(inverted)
  , m_current(current)
  {
  }

  template <typename T>
  void
  operator()(const T&) const
  {
  }

  void
  operator()(size_t rule) const
  {
    if (m_inverted.size() < rule + 1)
    {
      m_inverted.resize(rule + 1);
    }

    m_inverted[rule].push_back(m_current);
  }

  private:
  const std::unordered_map<size_t, earley::RuleList>& m_rules;
  std::vector<std::vector<const Rule*>>& m_inverted;
  const Rule* m_current;
};

std::vector<bool>
find_nullable(const std::unordered_map<size_t, earley::RuleList>& rules)
{
  std::vector<bool> nullable;
  std::deque<size_t> work;
  std::vector<std::vector<const Rule*>> inverted;

  for (auto& nt: rules)
  {
    if (nullable.size() < nt.first + 1)
    {
      nullable.resize(nt.first + 1);

      for (auto& rule : nt.second)
      {
        //empty rules
        if (rule.begin() != rule.end() &&
            std::holds_alternative<earley::Epsilon>(*rule.begin()))
        {
          nullable[nt.first] = true;
          work.push_back(nt.first);
        }
        else
        {
          nullable[nt.first] = false;
        }

        InvertVisitor invert_rule(rules, inverted, &rule);
        //inverted index
        for (auto& entry: rule)
        {
          std::visit(invert_rule, entry);
        }
      }
    }
  }

  while (work.size() != 0)
  {
    auto symbol = work.front();
    auto& work_rules = inverted[symbol];
    for (auto& wr: work_rules)
    {
      if (nullable[wr->nonterminal()])
      {
        continue;
      }

      bool next = false;
      for (auto& entry: *wr)
      {
        if (std::holds_alternative<size_t>(entry) &&
            nullable[std::get<size_t>(entry)])
        {
          next = true;
          break;
        }
      }

      if (next)
      {
        continue;
      }

      nullable[wr->nonterminal()] = true;
      work.push_back(wr->nonterminal());
    }

    work.pop_front();
  }

  return nullable;
}
