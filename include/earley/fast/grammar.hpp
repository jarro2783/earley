#include "earley.hpp"

namespace earley::fast::grammar
{
  struct Symbol
  {
    int index;
    bool terminal;
  };

  class Grammar
  {
    public:

    void
    insert_nonterminal(int index, std::vector<std::vector<Symbol>> rules);
  };

  class NonterminalIndices
  {
    public:

    int
    index(const std::string& name);

    int m_next = 0;
    std::unordered_map<std::string, int> m_names;
  };

  typedef std::unordered_map<std::string, size_t> TerminalIndices;

  Grammar
  build_grammar(const ::earley::Grammar& grammar);

  std::vector<std::vector<Symbol>>
  build_nonterminal
  (
    NonterminalIndices& nonterminal_indices,
    const TerminalIndices& terminals,
    const std::string& name,
    const std::vector<RuleWithAction>& rules
  );

  Symbol
  build_symbol
  (
    NonterminalIndices& nonterminal_indices,
    const TerminalIndices& terminals,
    const ::earley::Production& grammar_symbol
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
