#include <deque>

#include "earley.hpp"

namespace earley::fast::grammar
{
  struct Symbol
  {
    int index;
    bool terminal;
  };

  class Rule
  {
    public:

    Rule(int nonterminal, std::vector<Symbol> symbols)
    : m_nonterminal(nonterminal)
    , m_entries(std::move(symbols))
    {
    }

    int
    nonterminal() const
    {
      return m_nonterminal;
    }

    std::vector<Symbol>::const_iterator
    begin() const
    {
      return m_entries.begin();
    }

    std::vector<Symbol>::const_iterator
    end() const
    {
      return m_entries.end();
    }

    private:

    int m_nonterminal;
    std::vector<Symbol> m_entries;
    ActionArgs m_actions;
  };

  typedef std::vector<Rule> RuleList;

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

    Grammar(const std::string& start, const ::earley::Grammar& bnf);

    const RuleList&
    rules(const std::string& name);

    const RuleList&
    rules(int id)
    {
      return m_nonterminal_rules[id];
    }

    bool
    nullable(int nonterminal)
    {
      return m_nullable[nonterminal];
    }

    int
    start()
    {
      return m_start;
    }

    private:
    void
    insert_nonterminal(
      int index,
      const std::string& name,
      std::vector<Rule> rules
    );

    std::vector<Rule>
    build_nonterminal
    (
      int index,
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
    int m_start;
    std::vector<bool> m_nullable;

    NonterminalIndices m_nonterminal_indices;
    TerminalIndices m_terminals;

    //FirstSet m_first_sets;
    //FollowSet m_first_sets;
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

  inline
  void
  invert_rule(
    std::vector<std::vector<const Rule*>>& inverted,
    const Rule* current,
    const Symbol& symbol
  )
  {
    if (!symbol.terminal)
    {
      if (inverted.size() < static_cast<size_t>(symbol.index + 1))
      {
        inverted.resize(symbol.index + 1);
      }

      inverted[symbol.index].push_back(current);
    }
  }

  inline
  std::vector<bool>
  find_nullable(const std::vector<RuleList>& rules)
  {
    std::vector<bool> nullable(rules.size(), false);
    std::deque<int> work;
    std::vector<std::vector<const Rule*>> inverted;

    size_t i = 0;
    for (auto& nt: rules)
    {
      for (auto& rule : nt)
      {
        //empty rules
        if (rule.begin() == rule.end())
        {
          nullable[i] = true;
          work.push_back(i);
        }

        //inverted index
        for (auto& entry: rule)
        {
          invert_rule(inverted, &rule, entry);
        }
      }
      ++i;
    }

    // Ensure that there is an entry for every rule
    inverted.resize(rules.size());

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
          if (entry.terminal ||
              !nullable[entry.index])
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
}
