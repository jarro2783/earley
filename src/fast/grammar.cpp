#include <cassert>
#include "earley/fast/grammar.hpp"

namespace earley::fast::grammar
{

Grammar::Grammar(const std::string& start, const ::earley::Grammar& grammar)
{
  for (auto& [name, rules]: grammar)
  {
    auto index = m_nonterminal_indices.index(name);
    insert_nonterminal(index, name,
      build_nonterminal(index, rules));
  }

  // Add a special start rule to make things easier
  std::string start_name("^");
  auto start_index = m_nonterminal_indices.index(start_name);
  m_indices.insert({start_name, start_index});
  m_names.insert({start_index, start_name});

  m_start = start_index;

  if (static_cast<size_t>(start_index) >= m_nonterminal_rules.size())
  {
    m_nonterminal_rules.resize(start_index+1);
  }
  m_nonterminal_rules[start_index].push_back(
    {start_index, {{m_indices[start], false}}}
  );

  m_nullable = find_nullable(m_nonterminal_rules);

  // TODO: calculate first sets with the new rule class
}

void
Grammar::insert_nonterminal(
  int index,
  const std::string& name,
  std::vector<Rule> rules
)
{
  m_indices.insert({name, index});
  m_names.insert({index, name});

  assert(index >= 0);

  if (m_nonterminal_rules.size() <= static_cast<size_t>(index))
  {
    m_nonterminal_rules.resize(index+1);
  }

  m_nonterminal_rules[index] = std::move(rules);
}

const RuleList&
Grammar::rules(const std::string& name)
{
  auto iter = m_indices.find(name);

  if (iter == m_indices.end())
  {
    throw name + " not found";
  }

  return m_nonterminal_rules.at(iter->second);
}

int
NonterminalIndices::index(const std::string& name)
{
  auto iter = m_names.find(name);

  if (iter != m_names.end())
  {
    return iter->second;
  }

  return m_names.insert({name, m_next++}).first->second;
}

Symbol
Grammar::build_symbol
(
  const ::earley::Production& grammar_symbol
)
{
  // A symbol can be a name or a character.
  // A character is always a terminal.
  // A name could be another non-terminal or a declared terminal.
  if (holds<std::string>(grammar_symbol))
  {
    auto& symbol_name = get<std::string>(grammar_symbol);
    if (is_terminal(m_terminals, symbol_name))
    {
      return Symbol
      {
        terminal_index(m_terminals, symbol_name),
        true,
      };
    }
    else
    {
      return Symbol
      {
        m_nonterminal_indices.index(symbol_name),
        false,
      };
    }
  }
  else
  {
    // it must hold a character
    return Symbol{get<char>(grammar_symbol), true};
  }
}

std::vector<Rule>
Grammar::build_nonterminal
(
  int index,
  const std::vector<RuleWithAction>& rules
)
{
  std::vector<Rule> nonterminal;
  for (auto& rule: rules)
  {
    std::vector<Symbol> symbols;
    for (auto& gsym: rule.productions())
    {
      if (holds<Scanner>(gsym))
      {
        throw "Unsupported Scanner for symbol";
      }

      symbols.push_back(build_symbol(gsym));
    }

    nonterminal.push_back(Rule(index, std::move(symbols)));
  }

  return nonterminal;
}

std::unordered_map<size_t, std::unordered_set<int>>
first_sets(const std::vector<RuleList>& rules)
{
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
          if (entry.terminal)
          {
            changed |= insert_value(entry.index, set);
            break;
          }
          else
          {
            auto next = entry.index;
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

}
