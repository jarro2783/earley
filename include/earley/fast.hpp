#ifndef EARLEY_FAST_HPP_INCLUDED
#define EARLEY_FAST_HPP_INCLUDED

#include <deque>
#include <memory>
#include <vector>

#include "earley.hpp"
#include "earley_hash_set.hpp"

#include "earley/grammar_util.hpp"
#include "earley/stack.hpp"

#include "earley/fast/grammar.hpp"
#include "earley/fast/items.hpp"

#define MAX_LOOKAHEAD_SETS 4

namespace earley::fast
{
  struct ItemSetOwner;
}

namespace earley
{
  namespace fast
  {
    typedef Item PItem;
    typedef grammar::Rule PRule;
    class CoreEqual;
    class CoreHash;

    typedef std::vector<size_t> TerminalList;

    // TODO: move this to a utility class
    template <typename T>
    class Range
    {
      public:
      Range(T b, T e)
      : m_begin(b)
      , m_end(e)
      {
      }

      T
      begin() const
      {
        return m_begin;
      }

      T
      end() const
      {
        return m_end;
      }

      auto
      operator[](int i) const
      {
        return *(m_begin + i);
      }

      private:
      T m_begin;
      T m_end;
    };

    template <typename T>
    Range<T>
    make_range(T begin, T end)
    {
      return Range(begin, end);
    }

    inline
    auto
    is_terminal(const earley::Entry& s)
    {
      return s.terminal();
    }

    inline
    auto
    is_terminal(const grammar::Symbol& s)
    {
      return s.terminal;
    }

    inline
    auto
    get_terminal(const earley::Entry& e)
    {
      return get<size_t>(e);
    }

    inline
    auto
    get_symbol(const earley::Entry& e)
    {
      return get<size_t>(e);
    }

    inline
    auto
    get_symbol(const grammar::Symbol& s)
    {
      return s;
    }

    inline
    auto
    get_terminal(const grammar::Symbol& s)
    {
      return s.index;
    }

    class ItemSetCore
    {
      public:

      ItemSetCore()
      {
        //m_parent_indexes.reserve(4);
        m_parent_list_end = m_parent_list = parent_stack.start();
        m_item_list_end = m_item_list = item_stack.start();
      }

      void
      number(int n)
      {
        m_number = n;
      }

      int
      number() const
      {
        return m_number;
      }

      void
      add_start_item(const PItem* item)
      {
        ++m_start_items;
        insert_item(item);
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
        auto size = m_item_list_end - m_item_list;
        return size;
      }

      void
      add_derived_item(const PItem* item, size_t parent)
      {
        insert_item(item);
        //m_parent_indexes.push_back(parent);
        m_parent_list = parent_stack.emplace_back(parent);
        m_parent_list_end = m_parent_list + parent_stack.top_size();
      }

      void
      add_initial_item(const PItem* item)
      {
        insert_item(item);
      }

      const PItem*
      item(size_t i) const
      {
        return m_item_list[i];
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      auto
      items() const
      {
        return make_range(m_item_list, m_item_list_end);
      }

      const Item**
      first_item() const
      {
        return m_item_list;
      }

      size_t
      all_distances() const
      {
        //return m_start_items + m_parent_indexes.size();
        return m_start_items + (m_parent_list_end - m_parent_list);
      }

      size_t
      parent_distance(size_t distance)
      {
        //return m_parent_indexes.at(distance - m_start_items);
        return m_parent_list[distance - m_start_items];
      }

      auto
      parent_distances() const
      {
        //return m_parent_indexes;
        return m_parent_list;
      }

      void
      reset()
      {
        //m_parent_indexes.resize(0);
        m_start_items = 0;
        m_hash = 0;

        item_stack.destroy_top();
        m_item_list_end = m_item_list;

        parent_stack.destroy_top();
        m_parent_list_end = m_parent_list;
      }

      // There will be no more changes to this set.
      void
      finalise()
      {
        item_stack.finalise();
        parent_stack.finalise();
      }

      private:
      void
      insert_item(const Item* item)
      {
        m_item_list = item_stack.emplace_back(item);
        m_item_list_end = m_item_list + item_stack.top_size();
      }

      size_t m_start_items = 0;
      size_t m_hash = 0;
      //std::vector<size_t> m_parent_indexes;

      int* m_parent_list = nullptr;
      int *m_parent_list_end = nullptr;
      static Stack<int> parent_stack;

      const Item** m_item_list = nullptr;
      const Item** m_item_list_end = nullptr;
      static Stack<const Item*> item_stack;

      int m_number;
    };

    class ItemSet
    {
      public:

      ItemSet(ItemSetCore* core)
      : m_core(core)
      , m_distances_end(nullptr)
      {
        //m_distances.reserve(10);
        m_distances = distance_stack.start();
      }

      ItemSet(const ItemSet&) = delete;
      ItemSet(ItemSet&&) = default;

      // This should only be used when the core is equivalent to the
      // existing one
      void
      set_core(ItemSetCore* core)
      {
        m_core = core;
      }

      void
      add_start_item(const PItem* item, size_t distance);

      void
      add_derived_item(const PItem* item, size_t parent)
      {
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

      const auto
      distances() const
      {
        return make_range(m_distances, 
          m_distances_end == 0 ? m_distances + distance_stack.top_size() :
          m_distances_end);
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
        else if (item < m_core->all_distances())
        {
          return m_distances[m_core->parent_distance(item)];
        }
        else
        {
          return 0;
        }
      }

      void
      finalise()
      {
        m_distances_end = m_distances + distance_stack.top_size();
        distance_stack.finalise();
      }

      void
      print(const std::unordered_map<size_t, std::string>& names) const;

      void
      reset(ItemSetCore* core)
      {
        m_core = core;
        //m_distances.resize(0);
        m_hash = 0;
        distance_stack.destroy_top();
      }

      private:
      ItemSetCore* m_core;
      //std::vector<size_t> m_distances;
      size_t m_hash = 0;

      int* m_distances;
      int* m_distances_end;

      static Stack<int> distance_stack;
    };

    class ItemSetOwner
    {
      public:
      ItemSetOwner(ItemSet* set)
      : m_set(set)
      {
      }

      ItemSetOwner(ItemSetOwner&&) = default;
      ItemSetOwner(const ItemSetOwner&) = delete;

      auto&
      get() const
      {
        return *m_set;
      }

      private:
      ItemSet* m_set;
    };

    inline
    bool
    operator==(const ItemSetOwner& lhs, const ItemSetOwner& rhs)
    {
      auto slhs = &lhs.get();
      auto srhs = &rhs.get();

      if (slhs == srhs)
      {
        return true;
      }

      auto pl = slhs;
      auto pr = srhs;

      if (pl->core() != pr->core())
      {
        return false;
      }

      // we also have to compare the distances
      auto& dl = pl->distances();
      auto& dr = pr->distances();

      // they have the same number of start items here,
      // so their distances must have at least start_items elements
      if (!std::equal(dl.begin(), dl.begin() + pl->core()->start_items(), dr.begin()))
      {
        return false;
      }

      return true;
    }

    struct SetSymbolRules
    {
      SetSymbolRules(
        ItemSetCore* _set,
        grammar::Symbol _symbol
      )
      : set(_set)
      , symbol(_symbol)
      {
      }

      ItemSetCore* set;
      grammar::Symbol symbol;
      std::vector<uint16_t> transitions;
    };

    struct SetTermLookahead
    {
      SetTermLookahead(
        ItemSet* _set,
        int _symbol,
        int _lookahead
      )
      : set(_set)
      , symbol(_symbol)
      , lookahead(_lookahead)
      {
      }

      ItemSet* set;
      int symbol;
      int lookahead;

      ItemSet* goto_sets[MAX_LOOKAHEAD_SETS];
      int place[MAX_LOOKAHEAD_SETS] = {0};
      int goto_position = 0;
      int goto_count = 0;
    };

    inline
    bool
    operator==(const SetTermLookahead& lhs, const SetTermLookahead& rhs)
    {
      return lhs.set == rhs.set && lhs.symbol == rhs.symbol &&
        lhs.lookahead == rhs.lookahead;
    }

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

    struct ItemTreePointers
    {
      ItemTreePointers(
        const Item* _source,
        const ItemSet* _from,
        size_t _label
      )
      : source(_source)
      , from(_from)
      , label(_label)
      {
      }

      const Item* source;
      const ItemSet* from;
      size_t label;

      template <typename T>
      using Container = std::vector<T>;

      Container<const Item*> reduction;
      Container<const Item*> predecessor;
    };

    struct ItemMembership
    {
      const Item* item;
      std::vector<int> dot_is_member;

      ItemMembership(const Item* _item)
      : item(_item)
      {
      }

      void
      set_entry(size_t position, int value);

      int
      get_entry(size_t position);
    };

    class Parser
    {
      public:
      typedef HashSet<SetSymbolRules> SetSymbolHash;
      typedef HashSet<SetTermLookahead> SetTermLookaheadHash;
      typedef HashSet<ItemTreePointers> ItemTreeHash;
      typedef HashSet<ItemMembership> ItemMembershipHash;

      Parser(const grammar::Grammar&, const TerminalList&);

      void
      parse_input();

      void
      parse(size_t position);

      void
      print_set(size_t i);

      void
      print_stats() const;

      private:

      void
      create_all_items();

      void
      create_start_set();

      const earley::Item*
      get_item(const earley::Rule* rule, size_t dot) const;

      const Item*
      get_item(const grammar::Rule* rule, int dot);

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

      void
      insert_transitions(ItemSetCore*, const Entry& symbol, size_t);

      void
      insert_transitions(ItemSetCore*, const grammar::Symbol&, size_t);

      SetSymbolHash::iterator
      new_symbol_index(const SetSymbolRules& item)
      {
        return m_set_symbols.insert(item).first;
      }

      ItemSet*
      create_new_set(size_t position, const TerminalList& input);

      bool
      nullable(const Entry& symbol)
      {
        return symbol.empty();
      }

      bool
      nullable(const grammar::Symbol& symbol)
      {
        return !symbol.terminal && m_grammar_new.nullable(symbol.index);
      }

      void
      parse_error(size_t);

      bool
      item_in_set(const ItemSet* set, const Item* item, int distance,
        int position);

      void
      unique_insert_start_item(
        ItemSet* set,
        const Item* item,
        int distance,
        int position
      );

      ItemSetCore&
      next_core();

      ItemSet&
      next_set(ItemSetCore* core);

      void
      reset_set();

      grammar::Grammar m_grammar_new;
      const TerminalList& m_tokens;

      std::vector<ItemSet*> m_itemSets;
      HashSet<ItemSetOwner> m_item_set_hash;
      HashSet<ItemSetCore*, CoreHash, CoreEqual> m_set_core_hash;
      //std::deque<ItemSet> m_setOwner;
      //std::deque<ItemSetCore> m_coreOwner;

      std::vector<ItemSet> m_setOwner;
      std::vector<ItemSetCore> m_coreOwner;

      bool m_core_reset = false;
      bool m_set_reset = false;

      // The addresses of these might change after adding another one, so only
      // keep a pointer to them after adding all the items
      // TODO: this is going away
      std::unordered_map<const earley::Rule*, std::vector<earley::Item>> m_items;

      SetSymbolHash m_set_symbols;
      SetTermLookaheadHash m_set_term_lookahead;
      ItemTreeHash m_item_tree;

      std::vector<bool> m_nullable;

      std::unordered_map<size_t, std::string> m_names;

      // follow and first sets
      FirstSet m_first_sets;
      FollowSet m_follow_sets;

      Items m_all_items;

      std::vector<std::vector<size_t>> m_item_membership;

      int m_lookahead_collisions = 0;
      int m_reuse = 0;

      auto
      insert_transition(const SetSymbolRules& tuple)
      -> std::tuple<bool, decltype(m_set_symbols.find(tuple))>;
    };

    inline
    bool
    operator==(const ItemTreePointers& lhs, const ItemTreePointers& rhs)
    {
      return lhs.source == rhs.source;
    }

    inline
    bool
    operator==(const ItemMembership& lhs, const ItemMembership& rhs)
    {
      return lhs.item == rhs.item;
    }

    inline
    void
    ItemMembership::set_entry(size_t position, int value)
    {
      if (dot_is_member.size() <= position)
      {
        dot_is_member.resize(position + 1);
      }

      dot_is_member[position] = value;
    }

    inline
    int
    ItemMembership::get_entry(size_t position)
    {
      if (dot_is_member.size() <= position)
      {
        return -1;
      }
      else
      {
        return dot_is_member[position];
      }
    }

    struct CoreHash
    {
      size_t
      operator()(const ItemSetCore* core)
      {
        return core->hash();
      }
    };

    struct CoreEqual
    {
      bool
      operator()(const ItemSetCore* lhs, const ItemSetCore* rhs)
      {
        return
          lhs->start_items() == rhs->start_items() &&
          std::equal(
            lhs->first_item(),
            lhs->first_item() + lhs->start_items(),
            rhs->first_item())
        ;
      }
    };
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

  template <>
  struct hash<earley::fast::SetSymbolRules>
  {
    size_t
    operator()(const earley::fast::SetSymbolRules& s)
    {
      size_t result = std::hash<decltype(s.set)>()(s.set);
      size_t entry_hash = std::hash<decltype(s.symbol)>()(s.symbol);
      hash_combine(result, entry_hash);

      return result;
    }
  };

  template <>
  struct hash<earley::fast::SetTermLookahead>
  {
    size_t
    operator()(const earley::fast::SetTermLookahead& s)
    {
      size_t result = std::hash<decltype(s.set)>()(s.set);
      size_t entry_hash = std::hash<decltype(s.symbol)>()(s.symbol);
      size_t lookahead_hash = std::hash<decltype(s.lookahead)>()(s.lookahead);

      hash_combine(result, entry_hash);
      hash_combine(result, lookahead_hash);

      return result;
    }
  };

  template <>
  struct hash<earley::fast::ItemSetOwner>
  {
    size_t
    operator()(const earley::fast::ItemSetOwner& s) const
    {
      return s.get().hash();
    }
  };

  template <>
  struct hash<earley::fast::ItemTreePointers>
  {
    size_t
    operator()(const earley::fast::ItemTreePointers& s) const
    {
      std::hash<decltype(s.source)> h;
      return h(s.source);
    }
  };

  template <>
  struct hash<earley::fast::ItemMembership>
  {
    size_t
    operator()(const earley::fast::ItemMembership& i) const
    {
      return std::hash<decltype(i.item)>()(i.item);
    }
  };
}

#endif
