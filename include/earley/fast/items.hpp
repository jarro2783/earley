#ifndef EARLEY_FAST_ITEMS_HPP_INCLUDED
#define EARLEY_FAST_ITEMS_HPP_INCLUDED

#include <exception>
#include <memory>

#include "earley/fast/grammar.hpp"

namespace earley::fast
{
  class NoSuchItem : public std::exception
  {
  };

  class Item
  {
    public:

    Item
    (
      const grammar::Rule* rule,
      grammar::Rule::iterator position,
      HashSet<int> lookahead,
      bool empty = false
    )
    : m_rule(rule)
    , m_position(position)
    , m_lookahead(std::move(lookahead))
    , m_empty_rhs(empty)
    {
    }

    auto
    position() const
    {
      return m_position;
    }

    const grammar::Rule&
    rule() const
    {
      return *m_rule;
    }

    int
    nonterminal() const
    {
      return m_rule->nonterminal();
    }

    auto
    end() const
    {
      return m_rule->end();
    }

    auto
    dot() const
    {
      return position();
    }

    auto
    dot_index() const
    {
      return position() - m_rule->begin();
    }

    bool
    in_lookahead(int symbol) const
    {
      return m_lookahead.count(symbol) > 0;
    }

    bool
    empty_rhs() const
    {
      return m_empty_rhs;
    }

    std::ostream&
    print(std::ostream&, const std::unordered_map<size_t, std::string>&)
      const;

    private:
    const grammar::Rule* m_rule;
    grammar::Rule::iterator m_position;
    HashSet<int> m_lookahead;
    bool m_empty_rhs;
  };

  class Items
  {
    public:
    Items(const std::vector<grammar::RuleList>& rules,
      const grammar::FirstSets&,
      const grammar::FollowSets&,
      const std::vector<bool>&);

    const Item*
    get_item(const grammar::Rule*, int position);

    private:

    std::unordered_map<const grammar::Rule*, std::vector<std::shared_ptr<Item>>>
      m_item_map;

    const grammar::FirstSets& m_firsts;
    const grammar::FollowSets& m_follows;
    const std::vector<bool>& m_nullable;
  };
}

#endif
