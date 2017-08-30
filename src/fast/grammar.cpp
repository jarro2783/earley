#include "earley/fast/grammar.hpp"

namespace earley::fast::grammar
{

size_t
NonterminalIndices::index(const std::string& name)
{
  auto iter = m_names.find(name);

  if (iter != m_names.end())
  {
    return iter->second;
  }

  return m_names.insert({name, m_next++}).first->second;
}

Grammar
build_grammar(const ::earley::Grammar& grammar)
{
  Grammar result;
  NonterminalIndices nonterminals;
  TerminalIndices terminals;

  for (auto& [name, rules]: grammar)
  {
    build_nonterminal(nonterminals, terminals, name, rules);
  }

  return result;
}

void
build_nonterminal
(
  NonterminalIndices& nonterminal_indices,
  const TerminalIndices& terminals,
  const std::string& name,
  const std::vector<RuleWithAction>& rules
)
{
  for (auto& rule: rules)
  {
    for (auto& gsym: rule.productions())
    {
      if (holds<Scanner>(gsym))
      {
        throw "Unsupported Scanner for symbol";
      }

      // A symbol can be a name or a character.
      // A character is always a terminal.
      // A name could be another non-terminal or a declared terminal.
      if (holds<std::string>(gsym))
      {
        auto& symbol_name = get<std::string>(gsym);
        if (is_terminal(terminals, symbol_name))
        {
          Symbol symbol
          {
            terminal_index(terminals, symbol_name),
            true,
          };
        }
        else
        {
          Symbol symbol
          {
            nonterminal_indices.index(symbol_name),
            false,
          };
        }
      }
      else
      {
        // it must hold a character
        Symbol symbol{get<char>(gsym), true};
      }
    }
  }
}

}
