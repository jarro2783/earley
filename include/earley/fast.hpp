#ifndef EARLEY_FAST_HPP_INCLUDED
#define EARLEY_FAST_HPP_INCLUDED

#include <memory>
#include <vector>

#include "earley.hpp"
#include "earley_hash_set.hpp"

namespace earley::fast
{
  struct ItemSetOwner;
}

namespace earley
{
  namespace fast
  {
    // TODO: consolidate this into the proper Rule class when I fix it
    // for the slow implementation
    class Rule
    {
      public:
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
      nonterminal() const
      {
        return m_nonterminal;
      }

      private:
      size_t m_nonterminal;
      std::vector<Symbol> m_entries;
    };

    class Item
    {
      public:

      std::vector<Symbol>::const_iterator
      dot() const
      {
        return m_dot;
      }

      const Rule*
      rule() const
      {
        return m_rule;
      }

      private:
      Rule* m_rule;
      std::vector<Symbol>::const_iterator m_dot;
    };

    class ItemSetCore
    {
      public:

      void
      add_start_item(const Item* item)
      {
        ++m_start_items;
        m_items.push_back(item);
        hash_combine(m_hash, reinterpret_cast<size_t>(item));
      }

      size_t
      start_items() const
      {
        return m_start_items;
      }

      size_t
      all_items() const
      {
        return m_items.size();
      }

      void
      add_derived_item(const Item* item)
      {
        m_items.push_back(item);
      }

      const Item*
      item(size_t i)
      {
        return m_items[i];
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      const std::vector<const Item*>
      items() const
      {
        return m_items;
      }

      private:
      size_t m_start_items = 0;
      std::vector<const Item*> m_items;
      size_t m_hash = 0;
    };

    class ItemSet
    {
      public:

      ItemSet(ItemSetCore* core)
      : m_core(core)
      {
      }

      void
      add_start_item(const Item* item, size_t distance);

      void
      add_derived_item(const Item* item, size_t parent)
      {
        m_distances.push_back(parent);
        m_core->add_derived_item(item);
      }

      const ItemSetCore*
      core() const
      {
        return m_core;
      }

      ItemSetCore*
      core()
      {
        return m_core;
      }

      size_t
      distance(size_t i) const
      {
        return m_distances[i];
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      private:
      ItemSetCore* m_core;
      std::vector<size_t> m_distances;
      size_t m_hash = 0;
    };

    class ItemSetOwner
    {
      public:
      ItemSetOwner(const std::shared_ptr<ItemSet>& set)
      : m_set(set)
      {
      }

      auto
      get() const
      {
        return m_set;
      }

      private:
      std::shared_ptr<ItemSet> m_set;
    };

    inline
    bool
    operator==(const ItemSetOwner& lhs, const ItemSetOwner& rhs)
    {
      auto slhs = lhs.get();
      auto srhs = rhs.get();

      if (slhs == srhs)
      {
        return true;
      }

      auto pl = slhs.get();
      auto pr = srhs.get();

      // since sets are ordered the same we can just compare the
      // vectors
      if (pl->core()->start_items() != pr->core()->start_items())
      {
        return false;
      }

      size_t items = pl->core()->start_items();
      for (size_t i = 0; i != items; ++i)
      {
        if (pl->core()->item(i) != pr->core()->item(i))
        {
          return false;
        }
      }

      return true;
    }

    struct SetSymbolRules
    {
      ItemSetCore* set;
      Symbol symbol;
      std::vector<uint16_t> transitions;
    };

    inline
    bool
    operator==(const SetSymbolRules& lhs, const SetSymbolRules& rhs)
    {
      return lhs.set == rhs.set && lhs.symbol == rhs.symbol;
    }

    inline
    bool
    operator!=(const SetSymbolRules& lhs, const SetSymbolRules& rhs)
    {
      return !operator==(lhs, rhs);
    }

    class Parser
    {
      public:

      Parser(const ParseGrammar& grammar);

      void
      parse(const std::string& input);

      private:

      void
      create_start_set();

      const Item*
      get_item(const earley::Rule* rule, size_t dot) const;

      const Item*
      get_item(const Rule* rule, size_t dot) const;

      void
      expand_set(ItemSet* items);

      void
      add_empty_symbol_items(ItemSet* items);

      void
      add_non_start_items(ItemSet* items);

      void
      add_initial_item(ItemSetCore*, const Item* item);

      void
      item_transition(ItemSet* items, const Item* item, size_t i);

      HashSet<SetSymbolRules>::iterator
      new_symbol_index(const SetSymbolRules& item)
      {
        return m_set_symbols.insert(item).first;
      }

      std::shared_ptr<ItemSet>
      create_new_set(size_t position, char token);

      const ParseGrammar& m_grammar;
      std::vector<ItemSet> m_itemSets;
      HashSet<ItemSetOwner> m_item_set_hash;

      // The addresses of these might change after adding another one, so only
      // keep a pointer to them after adding all the items
      std::unordered_map<const Rule*, std::vector<Item>> m_items;
      HashSet<SetSymbolRules> m_set_symbols;
    };
  }
}

namespace std
{
  template <>
  struct hash<earley::fast::SetSymbolRules>
  {
    size_t
    operator()(const earley::fast::SetSymbolRules& s)
    {
      size_t result = std::hash<decltype(s.set)>()(s.set);
      size_t entry_hash = std::hash<earley::Symbol>()(s.symbol);
      hash_combine(result, entry_hash);

      return result;
    }
  };

  template <>
  struct hash<earley::fast::ItemSetOwner>
  {
    size_t
    operator()(const earley::fast::ItemSetOwner& s) const
    {
      return s.get()->hash();
    }
  };
}

#endif
