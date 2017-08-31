#include "earley.hpp"

namespace earley::fast::grammar
{
  struct Symbol
  {
    int index;
    bool terminal;
  };

  typedef std::vector<std::vector<Symbol>> RuleList;

  class NonterminalIndices
  {
    public:

    int
    index(const std::string& name);

    int m_next = 0;
    std::unordered_map<std::string, int> m_names;
  };

  typedef std::unordered_map<std::string, size_t> TerminalIndices;

  class Grammar
  {
    public:

    Grammar(const ::earley::Grammar& bnf);

    const RuleList&
    rules(const std::string& name);

    private:
    void
    insert_nonterminal(
      int index,
      const std::string& name,
      std::vector<std::vector<Symbol>> rules
    );

    std::vector<std::vector<Symbol>>
    build_nonterminal
    (
      const std::vector<RuleWithAction>& rules
    );

    Symbol
    build_symbol
    (
      const ::earley::Production& grammar_symbol
    );

    std::unordered_map<std::string, int> m_indices;
    std::unordered_map<int, std::string> m_names;
    std::vector<RuleList> m_nonterminal_rules;

    NonterminalIndices m_nonterminal_indices;
    TerminalIndices m_terminals;
  };

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
