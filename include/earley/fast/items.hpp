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
      HashSet<int> lookahead
    )
    : m_rule(rule)
    , m_position(position)
    , m_lookahead(std::move(lookahead))
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

    private:
    const grammar::Rule* m_rule;
    grammar::Rule::iterator m_position;
    HashSet<int> m_lookahead;
  };

  class Items
  {
    public:
    Items(const std::vector<grammar::RuleList>& rules,
      const grammar::FirstSets&,
      const grammar::FollowSets&);

    const Item*
    get_item(const grammar::Rule*, int position);

    private:

    std::unordered_map<const grammar::Rule*, std::vector<std::shared_ptr<Item>>>
      m_item_map;

    const grammar::FirstSets& m_firsts;
    const grammar::FollowSets& m_follows;
  };
}

#endif
