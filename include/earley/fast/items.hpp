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
      const HashSet<int>& lookahead,
      bool empty = false,
      size_t index = 0
    )
    : m_rule(rule)
    , m_position(position)
    , m_empty_rhs(empty)
    , m_index(index)
    {
      for (auto symbol : lookahead)
      {
        if (symbol == END_OF_INPUT)
        {
          //continue;
        }

        if (m_lookahead.size() <= static_cast<size_t>(symbol))
        {
          m_lookahead.resize(symbol+1);
        }
        m_lookahead[symbol] = true;
      }
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
      return 
        (m_lookahead.size() > static_cast<size_t>(symbol) &&
        m_lookahead[symbol]);
    }

    bool
    empty_rhs() const
    {
      return m_empty_rhs;
    }

    size_t
    index() const
    {
      return m_index;
    }

    std::ostream&
    print(std::ostream&, const std::unordered_map<size_t, std::string>&)
      const;

    private:
    const grammar::Rule* m_rule;
    grammar::Rule::iterator m_position;
    std::vector<bool> m_lookahead;
    bool m_empty_rhs;
    size_t m_index;
  };

  typedef std::vector<Item> ItemStore;

  struct RuleItems
  {
    RuleItems(const grammar::Rule* _rule)
    : rule(_rule)
    {
    }

    const grammar::Rule* rule;
    ItemStore items;
  };

  struct RuleItemHash
  {
    size_t
    operator()(const RuleItems& ri)
    {
      return std::hash<decltype(ri.rule)>()(ri.rule);
    }
  };

  struct RuleItemEq
  {
    bool
    operator()(const RuleItems& lhs, const RuleItems& rhs)
    {
      return lhs.rule == rhs.rule;
    }
  };

  class Items
  {
    public:
    Items(const std::vector<grammar::RuleList>& rules,
      const grammar::FirstSets&,
      const grammar::FollowSets&,
      const std::vector<bool>&);

    const Item*
    get_item(const grammar::Rule* rule, int position)
    {
      auto store = find_rule(rule);
      if (store == nullptr)
      {
        throw NoSuchItem();
      }

      auto length = rule->end() - rule->begin();

      if (position > length)
      {
        throw NoSuchItem();
      }

      return &(*store)[position];
    }

    size_t
    items() const
    {
      return m_item_index;
    }

    private:

    void
    fill_to(const grammar::Rule* rule, ItemStore& items,
      size_t position);

    ItemStore&
    insert_rule(const grammar::Rule*);

    const ItemStore*
    find_rule(const grammar::Rule* rule);

    std::vector<ItemStore> m_rule_array;

    const grammar::FirstSets& m_firsts;
    const grammar::FollowSets& m_follows;
    const std::vector<bool>& m_nullable;
    size_t m_item_index = 0;
  };
}

#endif
