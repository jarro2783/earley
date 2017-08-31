#include "earley/fast/grammar.hpp"

namespace earley::fast::grammar
{

void
Grammar::insert_nonterminal(int index, std::vector<std::vector<Symbol>> rules)
{
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
build_symbol
(
  NonterminalIndices& nonterminal_indices,
  const TerminalIndices& terminals,
  const ::earley::Production& grammar_symbol
)
{
  // A symbol can be a name or a character.
  // A character is always a terminal.
  // A name could be another non-terminal or a declared terminal.
  if (holds<std::string>(grammar_symbol))
  {
    auto& symbol_name = get<std::string>(grammar_symbol);
    if (is_terminal(terminals, symbol_name))
    {
      return Symbol
      {
        terminal_index(terminals, symbol_name),
        true,
      };
    }
    else
    {
      return Symbol
      {
        nonterminal_indices.index(symbol_name),
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

Grammar
build_grammar(const ::earley::Grammar& grammar)
{
  Grammar result;
  NonterminalIndices nonterminals;
  TerminalIndices terminals;

  for (auto& [name, rules]: grammar)
  {
    auto index = nonterminals.index(name);
    auto symbol_lists = build_nonterminal(nonterminals, terminals, name, rules);
    result.insert_nonterminal(index, symbol_lists);
  }

  return result;
}

std::vector<std::vector<Symbol>>
build_nonterminal
(
  NonterminalIndices& nonterminal_indices,
  const TerminalIndices& terminals,
  const std::string& name,
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

      symbols.push_back(build_symbol(nonterminal_indices, terminals, gsym));
    }

    nonterminal.push_back(std::move(symbols));
  }

  return nonterminal;
}

}
