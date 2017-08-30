#include "earley/fast/grammar.hpp"

namespace earley::fast::grammar
{

Grammar
build_grammar(const ::earley::Grammar& grammar)
{
  Grammar result;

  for (auto& [name, rules]: grammar)
  {
    build_nonterminal(result, name, rules);
  }

  return result;
}

void
build_nonterminal
(
  Grammar& grammar,
  const std::string& name,
  const std::vector<RuleWithAction>& rules
)
{
  for (auto& rule: rules)
  {
  }
}

}
