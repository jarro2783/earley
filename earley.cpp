#include <chrono>
#include <deque>
#include <iostream>
#include <unordered_map>

#include "earley.hpp"

using namespace earley;

namespace earley
{

ItemSetList
invert_items(const ItemSetList& item_sets)
{
  ItemSetList inverted(item_sets.size());

  size_t i = 0;
  for (auto& set: item_sets)
  {
    for (auto& item: set)
    {
      // We only need to keep completed items
      if (item.position() == item.end())
      {
        inverted[item.where()].insert(Item(item.rule(), i));
      }
    }
    ++i;
  }

  return inverted;
}

namespace
{
  template <typename T>
  void
  check_size(T& t, size_t s)
  {
    if (t.size() < s + 1)
    {
      t.resize(s+1);
    }
  }

  struct ItemCompare
  {
    bool
    operator()(const Item& lhs, const Item& rhs) const
    {
      return ((lhs.end() - lhs.rule().begin()) > (rhs.end() - rhs.rule().begin())) ||
        (lhs.where() > rhs.where()) ||
        (&lhs.rule() > &rhs.rule())
      ;
    }
  };
}

std::vector<std::vector<std::vector<Item>>>
sorted_index(const ItemSetList& item_sets)
{
  std::vector<std::vector<std::vector<Item>>> sorted;

  size_t i = 0;
  for (auto& item_set: item_sets)
  {
    check_size(sorted, i);

    auto& set_indexed = sorted[i];

    size_t j = 0;
    for (auto& item: item_set)
    {
      size_t nonterminal = item.nonterminal();
      check_size(set_indexed, nonterminal);

      set_indexed[nonterminal].push_back(item);

      ++j;
    }

    for (j = 0; j != set_indexed.size(); ++j)
    {
      auto& items = set_indexed[j];
      std::sort(items.begin(), items.end(), ItemCompare());
    }

    ++i;
  }

  return sorted;
}

}

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
  operator()(char c) const
  {
    return scan_char(c);
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

std::tuple<std::vector<RuleList>,
  std::unordered_map<std::string, size_t>>
generate_rules(Grammar& grammar)
{
  size_t next_id = 0;
  std::unordered_map<std::string, size_t> identifiers;
  std::vector<RuleList> rule_set;

  for (auto& nonterminal : grammar)
  {
    RuleList rules;
    auto id = rule_id(identifiers, next_id, nonterminal.first);

    for (auto& rule_action : nonterminal.second)
    {
      std::vector<Entry> entries;
      for (auto& production : rule_action.productions())
      {
        entries.push_back(make_entry(production, identifiers, next_id));
      }
      Rule rule(id, entries, rule_action.arguments());
      rules.push_back(rule);
    }

    if (rule_set.size() <= id)
    {
      rule_set.resize(id+1);
    }
    rule_set[id] = std::move(rules);
  }

  return std::make_tuple(rule_set, identifiers);
}

struct RecogniseActions
{
  RecogniseActions(
    const std::vector<RuleList>& rules,
    const std::vector<bool>& nullable,
    std::vector<Item>& stack,
    Item item,
    std::vector<ItemSet>& item_sets,
    size_t which,
    const std::string& input)
  : m_rules(rules)
  , m_nullable(nullable)
  , m_stack(stack)
  , m_item(item)
  , m_item_sets(item_sets)
  , m_which(which)
  , m_input(input)
  {
  }

  void
  operator()(size_t rule) const
  {
    // Predict
    // if it holds a non-terminal, add entries that expect the
    // non terminal
    auto& nt = m_rules[rule];
    for (auto& def : nt)
    {
      auto predict = Item(def, m_which);
      if (m_item_sets[m_which].insert(predict).second)
      {
        m_stack.push_back(predict);
      }
    }

    // nullable completion
    if (m_nullable[rule])
    {
      // we are completing *this* item, into the same set
      auto next = m_item.next();
      if (m_item_sets[m_which].insert(next).second)
      {
        m_stack.push_back(next);
      }
    }
  }

  void
  operator()(const Scanner& scan) const
  {
    // Scan
    // we scan the terminal and add it to the next set if it matches
    // if input[which] == item then advance
    if (m_which < m_input.length() && scan(m_input[m_which]))
    {
      m_item_sets[m_which+1].insert(m_item.next());
    }
  }

  void
  operator()(Epsilon) const
  {
    auto next = m_item.next();
    if (m_item_sets[m_which].insert(next).second)
    {
      m_stack.push_back(next);
    }
  }

  private:
  const std::vector<RuleList>& m_rules;
  const std::vector<bool>& m_nullable;
  std::vector<Item>& m_stack;
  Item m_item;
  std::vector<ItemSet>& m_item_sets;
  size_t m_which;
  const std::string& m_input;
};

void
complete(
  std::vector<Item>& stack,
  const Item& item,
  std::vector<ItemSet>& item_sets,
  size_t which
)
{
  // Complete
  // at the end of the item; a completion
  // we need to advance anything with our non-terminal to the right
  // of the current from where it was predicted into our current set
  auto ours = item.nonterminal();
  std::unordered_set<Item> to_add;

  for (auto& consider : item_sets[item.where()])
  {
    //std::cout << "Consider " << item << std::endl;
    auto dot = consider.position();
    if (dot != consider.end() &&
        std::holds_alternative<size_t>(*dot) &&
        std::get<size_t>(*dot) == ours)
    {
      //bring it into our set
      auto next = consider.next();
      if (item_sets[which].count(next) == 0 && to_add.count(next) == 0)
      {
        to_add.insert(next);
        stack.push_back(next);
      }
    }
  }

  item_sets[which].insert(to_add.begin(), to_add.end());
}

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
  const std::vector<earley::RuleList>& rules,
  const std::vector<bool>& nullable,
  size_t which
)
{
  std::vector<Item> to_process(item_sets[which].begin(), item_sets[which].end());

  while (!to_process.empty())
  {
    auto current = to_process.back();
    to_process.pop_back();

    auto pos = current.position();

    if (pos != current.end())
    {
      std::visit(
        RecogniseActions(rules, nullable, to_process,
                         current, item_sets, which, input),
        *pos);
    }
    else
    {
      complete(to_process, current, item_sets, which);
    }
  }
}

std::tuple<
  bool,
  double,
  ItemSetList
>
process_input(
  bool debug,
  size_t start,
  const std::string& input,
  const std::vector<RuleList>& rules
)
{
  ItemSetList item_sets(input.size() + 1);
  auto nullable = find_nullable(rules);

  if (debug)
  {
    std::cout << "Is nullable:" << std::endl;
    for (size_t i = 0; i != nullable.size(); ++i)
    {
      std::cout << i << ": " << nullable[i] << std::endl;
    }
  }

  auto& first_set = item_sets[0];

  for (auto& rule: rules[start])
  {
    first_set.insert(Item(rule));
  }

  std::chrono::time_point<std::chrono::system_clock> start_time, end;
  start_time = std::chrono::system_clock::now();

  for (size_t i = 0; i != input.size() + 1; ++i)
  {
    process_set(item_sets, input, rules, nullable, i);
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
    std::chrono::duration_cast<std::chrono::microseconds>(
      elapsed_seconds).count(),
    std::move(item_sets));
}

struct InvertVisitor
{
  InvertVisitor(const std::vector<earley::RuleList>& rules,
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
  const std::vector<earley::RuleList>& m_rules;
  std::vector<std::vector<const Rule*>>& m_inverted;
  const Rule* m_current;
};

std::vector<bool>
find_nullable(const std::vector<earley::RuleList>& rules)
{
  std::vector<bool> nullable(rules.size(), false);
  std::deque<size_t> work;
  std::vector<std::vector<const Rule*>> inverted;

  size_t i = 0;
  for (auto& nt: rules)
  {
    for (auto& rule : nt)
    {
      //empty rules
      if (rule.begin() != rule.end() &&
          std::holds_alternative<earley::Epsilon>(*rule.begin()))
      {
        nullable[i] = true;
        work.push_back(i);
      }

      InvertVisitor invert_rule(rules, inverted, &rule);
      //inverted index
      for (auto& entry: rule)
      {
        std::visit(invert_rule, entry);
      }
    }
    ++i;
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
