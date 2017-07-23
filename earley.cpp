#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
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

  template <template <typename, typename> typename Map, typename K, typename V>
  Map<V, K>
  invert_map(const Map<K, V>& map)
  {
    Map<V, K> inverted;

    for (auto& p: map)
    {
      inverted[p.second] = p.first;
    }

    return inverted;
  }

  void
  draw_pointers(const TreePointers::PointerList& pointers,
    const std::unordered_map<size_t, std::string>& names,
    std::ostream& out,
    const std::string& style
  )
  {
    size_t cluster = 0;
    for (auto& item_set: pointers)
    {
      for (auto& items: item_set)
      {
        for (auto& item_label: items.second)
        {
          out << "  \"";
          items.first.print(out, names);
          out << ":" << cluster << "\" -> \"";
          item_label.second.first.print(out, names);
          out << ":" << item_label.second.second << "\" [style=" << style
              << " label=\"" << item_label.first << "\"];\n";
        }
      }
      ++cluster;
    }
  }

  void
  dump_pointers(const ItemSetList& item_sets, const TreePointers& pointers,
      const std::unordered_map<size_t, std::string>& names)
  {
    std::ofstream out("graph");
    std::unordered_map<Item, size_t> nodes;

    out << "digraph {\n";

    size_t index = 0;
    for (auto& items: item_sets)
    {
      out << "subgraph cluster_" << index << " {\n";
      out << "  label = \"set " << index << "\";\n";
      for (auto& item: items)
      {
        out << "  \"";
        item.print(out, names);
        out << ":" << index << "\";\n";
      }
      out << "}\n";
      ++index;
    }

    draw_pointers(pointers.reductions(), names, out, "solid");
    draw_pointers(pointers.predecessors(), names, out, "dashed");

    out << "}";
  }
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
    const std::string& input,
    TreePointers& pointers,
    std::unordered_map<size_t, Item>& nulled)
  : m_rules(rules)
  , m_nullable(nullable)
  , m_stack(stack)
  , m_item(item)
  , m_item_sets(item_sets)
  , m_which(which)
  , m_input(input)
  , m_pointers(pointers)
  , m_nulled(nulled)
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
      std::cout << "Nullable prediction adding " << next << ":" << m_which 
                << " -> " << m_item << ":" << m_which << std::endl;
      m_pointers.predecessor(m_which, m_which, m_which, next, m_item);

      if (m_item_sets[m_which].insert(next).second)
      {
        m_stack.push_back(next);
      }

      auto nulled = m_nulled.find(rule);
      if (nulled != m_nulled.end())
      {
        std::cout << "Nulled prediction adding reduction " 
                  << next << ":" << m_which 
                  << " -> " << nulled->second << ":" << m_which << std::endl;
        // The item has already been added, we just add a reduction here
        m_pointers.reduction(m_which, m_which, next, nulled->second);
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
      std::cout << "Scan adding " << m_item.next() << ":" << m_which+1
                << " -> " << m_item << ":" << m_which << std::endl;
      m_pointers.predecessor(m_which+1, m_which, m_which, m_item.next(), m_item);
      m_item_sets[m_which+1].insert(m_item.next());
    }
  }

  void
  operator()(Epsilon) const
  {
    // this is a completion
    auto next = m_item.next();
    std::cout << "Epsilon adding reduction " 
              << next << ":" << m_which 
              << " -> " << m_item << ":" << m_which << std::endl;
    m_pointers.reduction(m_which, m_which, next, m_item);
    if (m_item_sets[m_which].insert(next).second)
    {
      m_stack.push_back(next);
    }

    // TODO: This isn't good enough, I actually need to 
    // deal with A -> B where B is nullable
    m_nulled.insert({m_item.nonterminal(), m_item.next()});
  }

  private:
  const std::vector<RuleList>& m_rules;
  const std::vector<bool>& m_nullable;
  std::vector<Item>& m_stack;
  Item m_item;
  std::vector<ItemSet>& m_item_sets;
  size_t m_which;
  const std::string& m_input;
  TreePointers& m_pointers;
  std::unordered_map<size_t, Item>& m_nulled;
};

void
complete(
  std::vector<Item>& stack,
  TreePointers& pointers,
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
      std::cout << "Completion adding reduction " 
                << next << ":" << which 
                << " -> " << item << ":" << which << std::endl;
      pointers.reduction(which, item.where(), next, item);

      if (dot != consider.rule().begin())
      {
        std::cout << "Completion adding " << next << ":" << which
                  << " -> " << consider << ":" << item.where() << std::endl;
        pointers.predecessor(which, item.where(), item.where(), next, consider);
      }
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
  TreePointers& pointers,
  const std::string& input,
  const std::vector<earley::RuleList>& rules,
  const std::vector<bool>& nullable,
  size_t which
)
{
  std::vector<Item> to_process(item_sets[which].begin(), item_sets[which].end());
  std::unordered_map<size_t, Item> nulled;

  while (!to_process.empty())
  {
    auto current = to_process.back();
    to_process.pop_back();

    auto pos = current.position();

    if (pos != current.end())
    {
      std::visit(
        RecogniseActions(rules, nullable, to_process,
                         current, item_sets, which, input, pointers, nulled),
        *pos);
    }
    else
    {
      complete(to_process, pointers, current, item_sets, which);
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
  const std::vector<RuleList>& rules,
  const std::unordered_map<std::string, size_t>& names
)
{
  ItemSetList item_sets(input.size() + 1);
  auto nullable = find_nullable(rules);

  auto rule_names = invert_map(names);

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

  TreePointers pointers;

  for (size_t i = 0; i != input.size() + 1; ++i)
  {
    process_set(item_sets, pointers, input, rules, nullable, i);
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
        item.print(std::cout, rule_names);
        std::cout << std::endl;
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
      dump_pointers(item_sets, pointers, rule_names);
      parsed = true;
      if (debug)
      {
        std::cout << "Parsed: " << input << std::endl;
        item.print(std::cout, rule_names);
        std::cout << std::endl;

        std::cout << "Pointers for top item" << std::endl;
        auto& reductions = pointers.reductions();
        if (reductions.size() > input.size())
        {
          auto& last = reductions[input.size()];

          for (auto& item_reduction: last)
          {
            for (auto& reduction: item_reduction.second)
            {
              std::cout << "Reduction from ";
              item_reduction.first.print(std::cout, rule_names);
              std::cout << " to ";
              reduction.second.first.print(std::cout, rule_names);
              std::cout << " labelled " << reduction.first << std::endl;
            }
          }

          auto iter = last.find(item);
          if (iter != last.end())
          {
            std::cout << "Last item pointer" << std::endl;
            for (auto& reduction: iter->second)
            {
              std::cout << "Reduction from ";
              item.print(std::cout, rule_names);
              std::cout << " to ";
              reduction.second.first.print(std::cout, rule_names);
              std::cout << " labelled " << reduction.first << std::endl;
            }
          }
          else
          {
            std::cout << "Item not found" << std::endl;
          }
        }
        else
        {
          std::cout << "No reduction at end" << std::endl;
        }
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
