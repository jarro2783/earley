#include "earley.hpp"

namespace earley::fast::grammar
{
  struct Symbol
  {
    size_t index;
    bool terminal;
  };

  class NonterminalIndices
  {
    public:

    size_t
    index(const std::string& name);

    size_t m_next = 0;
    std::unordered_map<std::string, size_t> m_names;
  };

  typedef std::unordered_map<std::string, size_t> TerminalIndices;

  Grammar
  build_grammar(const ::earley::Grammar& grammar);

  void
  build_nonterminal
  (
    NonterminalIndices& nonterminal_indices,
    const TerminalIndices& terminals,
    const std::string& name,
    const std::vector<RuleWithAction>& rules
  );

  inline
  bool
  is_terminal(const TerminalIndices& indices, const std::string& name)
  {
    return indices.count(name) != 0;
  }

  inline
  int
  terminal_index(const TerminalIndices& indices, const std::string& name)
  {
    auto iter = indices.find(name);

    return iter != indices.end() ? iter->second : -1;
  }
}
