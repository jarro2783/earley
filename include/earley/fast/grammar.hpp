#ifndef EARLEY_FAST_GRAMMAR_HPP_INCLUDED
#define EARLEY_FAST_GRAMMAR_HPP_INCLUDED

#include <deque>

#include "earley.hpp"
#include "earley/grammar_util.hpp"

namespace earley::fast::grammar
{
  struct Symbol
  {
    int index;
    bool terminal;
  };

  inline
  bool
  operator==(const Symbol& lhs, const Symbol& rhs)
  {
    return lhs.index == rhs.index &&
      lhs.terminal == rhs.terminal;
  }

  enum Terminals {
    EPSILON = -1,
    END_OF_INPUT = -2,
  };

  typedef std::unordered_map<size_t, std::unordered_set<int>> FirstSets;
  typedef std::unordered_map<size_t, std::unordered_set<int>> FollowSets;

  class Validation
  {
    public:

    Validation(
      std::vector<std::string> undefined
    )
    : m_undefined(std::move(undefined))
    {
      m_valid = 
        (m_undefined.size() == 0)
      ;
    }

    bool
    is_valid() const
    {
      return m_valid;
    }

    const std::vector<std::string>&
    undefined() const
    {
      return m_undefined;
    }

    private:
    bool m_valid;
    std::vector<std::string> m_undefined;
  };


  class Rule
  {
    public:

    Rule(int nonterminal, std::vector<Symbol> symbols)
    : m_nonterminal(nonterminal)
    , m_entries(std::move(symbols))
    {
      m_index = global_rule_counter++;
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

    size_t
    index() const
    {
      return m_index;
    }

    private:

    int m_nonterminal;
    std::vector<Symbol> m_entries;
    ActionArgs m_actions;
    size_t m_index;

    static size_t global_rule_counter;

    public:
    typedef decltype(m_entries)::const_iterator iterator;
  };

  HashSet<int>
  sequence_lookahead
  (
    const Rule& rule,
    Rule::iterator begin,
    const FirstSets&,
    const FollowSets&
  );

  typedef std::vector<Rule> RuleList;

  class NonterminalIndices
  {
    public:

    int
    index(const std::string& name);

    const std::unordered_map<std::string, int>&
    names() const
    {
      return m_names;
    }

    private:

    int m_next = 0;
    std::unordered_map<std::string, int> m_names;
  };

  typedef std::unordered_map<std::string, size_t> TerminalIndices;

  FirstSets
  first_sets(const std::vector<RuleList>& rules);

  FollowSets
  follow_sets(int start, const std::vector<RuleList>&, FirstSets&);

  class Grammar
  {
    public:

    Grammar(
      const std::string& start,
      const ::earley::Grammar& bnf,
      TerminalIndices terminals = {}
    );

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

    auto&
    first_sets() const
    {
      return m_first_sets;
    }

    auto&
    follow_sets() const
    {
      return m_follow_sets;
    }

    auto&
    all_rules() const
    {
      return m_nonterminal_rules;
    }

    auto&
    names() const
    {
      return m_names;
    }

    const std::vector<bool>&
    nullable_set() const
    {
      return m_nullable;
    }

    Validation
    validate() const;

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

    FirstSet m_first_sets;
    FollowSet m_follow_sets;
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

  template <typename Iterator,
    typename = std::enable_if<
      std::is_same<
        typename Iterator::value_type,
        Symbol
      >::value
    >
  >
  std::unordered_set<size_t>
  first_set(Iterator begin, Iterator end,
    const std::unordered_map<size_t, std::unordered_set<int>>& firsts)
  {
    std::unordered_set<size_t> result;

    while (begin != end)
    {
      auto& entry = *begin;

      if (entry.terminal)
      {
        insert_value(entry.index, result);
        break;
      }
      else
      {
        auto iter = firsts.find(entry.index);
        if (iter != firsts.end())
        {
          insert_range(iter->second.begin(), iter->second.end(), result);

          if (iter->second.count(EPSILON) == 0)
          {
            break;
          }
        }
      }

      ++begin;
    }

    if (begin == end)
    {
      result.insert(EPSILON);
    }
    else
    {
      // epsilon could have been added earlier, and we only want it if we
      // actually got to the end of the sequence
      result.erase(EPSILON);
    }

    return result;
  }
}

namespace std
{
  template <>
  struct hash<earley::fast::grammar::Symbol>
  {
    size_t
    operator()(const earley::fast::grammar::Symbol& s)
    {
      size_t result = s.index;
      hash_combine(result, s.terminal);

      return result;
    }
  };
}

#endif
