#include "earley.hpp"

namespace earley::fast::grammar
{
  Grammar
  build_grammar(const ::earley::Grammar& grammar);

  void
  build_nonterminal
  (
    Grammar& grammar,
    const std::string& name,
    const std::vector<RuleWithAction>& rules
  );
}
