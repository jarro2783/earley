#ifndef EARLEY_FAST_HPP_INCLUDED
#define EARLEY_FAST_HPP_INCLUDED

#include <memory>
#include <vector>

#include "earley.hpp"
#include "earley_hash_set.hpp"

#include "earley/grammar_util.hpp"
#include "earley/fast/grammar.hpp"

#include "earley/fast/grammar.hpp"
#include "earley/fast/items.hpp"

namespace earley::fast
{
  struct ItemSetOwner;
}

namespace earley
{
  namespace fast
  {
    typedef earley::Item PItem;

    class ItemSetCore
    {
      public:

      ItemSetCore()
      {
        m_items.reserve(100);
      }

      void
      add_start_item(const PItem* item)
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
      add_derived_item(const PItem* item, size_t parent)
      {
        m_items.push_back(item);
        m_parent_indexes.push_back(parent);
      }

      void
      add_initial_item(const PItem* item)
      {
        m_items.push_back(item);
      }

      const PItem*
      item(size_t i)
      {
        return m_items[i];
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      const std::vector<const PItem*>
      items() const
      {
        return m_items;
      }

      size_t
      all_distances() const
      {
        return m_start_items + m_parent_indexes.size();
      }

      size_t
      parent_distance(size_t distance)
      {
        return m_parent_indexes.at(distance - m_start_items);
      }

      private:
      size_t m_start_items = 0;
      std::vector<const PItem*> m_items;
      size_t m_hash = 0;
      std::vector<size_t> m_parent_indexes;
    };

    class ItemSet
    {
      public:

      ItemSet(ItemSetCore* core)
      : m_core(core)
      {
      }

      void
      add_start_item(const PItem* item, size_t distance);

      void
      add_derived_item(const PItem* item, size_t parent)
      {
        hash_combine(m_hash, std::hash<const PItem*>()(item));
        hash_combine(m_hash, parent + 123456789);

        m_distances.push_back(parent);
        m_core->add_derived_item(item, parent);
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
        if (i >= m_core->start_items())
        {
          return 0;
        }
        return m_distances[i];
      }

      const auto&
      distances() const
      {
        return m_distances;
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      size_t
      actual_distance(size_t item) const
      {
        if (item < m_core->start_items())
        {
          return m_distances[item];
        }
        else if (item < m_distances.size())
        {
          return m_distances[m_distances[item]];
        }
        else
        {
          return 0;
        }
      }

      void
      print(const std::unordered_map<size_t, std::string>& names) const;

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

      // we also have to compare the distances
      if (pl->distances() != pr->distances())
      {
        return false;
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

      Parser(const grammar::Grammar&, const ParseGrammar& grammar,
        std::unordered_map<size_t, std::string> names);

      void
      parse_input(const std::string& input);

      void
      parse(const std::string& input, size_t position);

      void
      print_set(size_t i, const std::unordered_map<size_t, std::string>&);

      private:

      void
      create_all_items();

      void
      create_start_set();

      const PItem*
      get_item(const earley::Rule* rule, size_t dot) const;

      void
      expand_set(ItemSet* items);

      void
      add_empty_symbol_items(ItemSet* items);

      void
      add_non_start_items(ItemSet* items);

      void
      add_initial_item(ItemSetCore*, const PItem* item);

      void
      item_transition(ItemSet* items, const PItem* item, size_t i);

      void
      item_completion(ItemSet*, const PItem*, size_t i);

      HashSet<SetSymbolRules>::iterator
      new_symbol_index(const SetSymbolRules& item)
      {
        return m_set_symbols.insert(item).first;
      }

      std::shared_ptr<ItemSet>
      create_new_set(size_t position, const std::string& input);

      void
      set_item_lookahead(PItem& item);

      bool
      nullable(const Entry& symbol)
      {
        return symbol.empty();
      }

      // TODO: fix these
      ParseGrammar m_grammar;
      grammar::Grammar m_grammar_new;

      std::vector<std::shared_ptr<ItemSet>> m_itemSets;
      HashSet<ItemSetOwner> m_item_set_hash;

      // The addresses of these might change after adding another one, so only
      // keep a pointer to them after adding all the items
      std::unordered_map<const earley::Rule*, std::vector<PItem>> m_items;
      HashSet<SetSymbolRules> m_set_symbols;
      std::vector<bool> m_nullable;

      std::unordered_map<size_t, std::string> m_names;

      // follow and first sets
      FirstSet m_first_sets;
      FollowSet m_follow_sets;

      auto
      insert_transition(const SetSymbolRules& tuple)
      -> std::tuple<bool, decltype(m_set_symbols.find(tuple))>;
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
