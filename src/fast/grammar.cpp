#include <cassert>
#include "earley/fast/grammar.hpp"

namespace earley::fast::grammar
{

void
Grammar::insert_nonterminal(
  int index,
  const std::string& name,
  std::vector<std::vector<Symbol>> rules
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

Grammar::Grammar(const ::earley::Grammar& grammar)
{
  for (auto& [name, rules]: grammar)
  {
    auto index = m_nonterminal_indices.index(name);
    auto symbol_lists = build_nonterminal(rules);
    insert_nonterminal(index, name, symbol_lists);
  }
}

std::vector<std::vector<Symbol>>
Grammar::build_nonterminal
(
  const std::vector<RuleWithAction>& rules
)
{
  std::vector<std::vector<Symbol>> nonterminal;

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

    nonterminal.push_back(std::move(symbols));
  }

  return nonterminal;
}

}
