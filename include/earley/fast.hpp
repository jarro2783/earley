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

namespace earley
{
  namespace fast
  {
    typedef Item PItem;
    typedef grammar::Rule PRule;
    struct CoreEqual;
    struct CoreHash;

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

      auto
      start_item_list() const
      {
        return make_range(m_item_list, m_item_list + m_start_items);
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

        ++m_resets;
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
      int m_resets = 0;
    };

    class StackDistances
    {
      public:
      StackDistances(Stack<int>& stack)
      : m_stack(&stack)
      {
        stack.start();
      }

      size_t
      size() const
      {
        if (m_end != nullptr)
        {
          return m_end - m_top;
        }
        else
        {
          return m_stack->top_size();
        }
      }

      void
      finalise()
      {
        m_end = m_top + m_stack->top_size();
        m_stack->finalise();
      }

      void
      reset()
      {
        m_stack->destroy_top();
        m_stack->finalise();
        m_top = nullptr;
        m_end = nullptr;
        m_hash = 2053222611;
      }

      void
      append(int v)
      {
        m_top = m_stack->emplace_back(v);
        //hash_combine(m_hash, v);
        m_hash = m_hash * 611 + v;
      }

      size_t
      hash() const
      {
        return m_hash;
      }

      int
      operator[](int p) const
      {
        return m_top[p];
      }

      int*
      begin() const
      {
        return m_top;
      }

      int*
      end() const
      {
        if (m_end != nullptr)
        {
          return m_end;
        }
        else
        {
          return m_top + m_stack->top_size();
        }
      }

      private:
      Stack<int>* m_stack;
      int* m_top = nullptr;
      int* m_end = nullptr;
      size_t m_hash = 2053222611;
    };

    inline
    bool
    operator==(const StackDistances& lhs, const StackDistances& rhs)
    {
      return lhs.begin() == rhs.begin();
    }

    inline
    bool
    operator!=(const StackDistances& lhs, const StackDistances& rhs)
    {
      return !operator==(lhs, rhs);
    }

    struct StackDistanceHash
    {
      size_t
      operator()(const StackDistances& s)
      {
        return s.hash();
      }
    };

    struct StackDistanceEq
    {
      bool
      operator()(const StackDistances& lhs, const StackDistances& rhs)
      {
        return lhs.size() == rhs.size() &&
          std::equal(lhs.begin(), lhs.end(), rhs.begin());
      }
    };

    class ItemSet
    {
      public:

      ItemSet(ItemSetCore* core)
      : m_core(core)
      , m_distances(distance_stack)
      {
        //m_distances.reserve(10);
        //m_distances = distance_stack.start();
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

      const auto&
      distances() const
      {
        return m_distances;
      }

      auto&
      distances()
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
        //m_distances.finalise();
        hash_combine(m_hash, m_distances.begin());
      }

      void
      print(const std::unordered_map<size_t, std::string>& names) const;

      void
      reset(ItemSetCore* core)
      {
        m_core = core;
        m_hash = 0;
        m_distances.reset();
      }

      void
      set_distance(const StackDistances& d)
      {
        distance_stack.destroy_top();
        m_distances = d;
      }

      private:
      ItemSetCore* m_core;
      size_t m_hash = 0;

      StackDistances m_distances;
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
      //auto& dl = pl->distances();
      //auto& dr = pr->distances();

      // they have the same number of start items here,
      // so their distances must have at least start_items elements
      //if (!std::equal(dl.begin(), dl.begin() + pl->core()->start_items(), dr.begin()))
      //{
      //  return false;
      //}

      if (pl->distances() != pr->distances())
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
        hash = std::hash<decltype(set)>()(set);
        size_t entry_hash = std::hash<decltype(symbol)>()(symbol);
        hash_combine(hash, entry_hash);
      }

      ItemSetCore* set;
      grammar::Symbol symbol;
      size_t hash;
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
      using Container = HashSet<T>;

      using LabelledItem = std::tuple<const Item*, size_t>;

      Container<LabelledItem> reduction;
      Container<LabelledItem> predecessor;
    };

    class Parser
    {
      public:
      typedef HashMap<SetSymbolRules, std::vector<uint16_t>> SetSymbolHash;
      typedef HashSet<SetTermLookahead> SetTermLookaheadHash;
      typedef HashSet<ItemTreePointers> ItemTreeHash;
      typedef HashSet<StackDistances, StackDistanceHash, StackDistanceEq>
        DistanceHash;

      Parser(const grammar::Grammar&, const TerminalList&);

      void
      parse_input();

      void
      parse(size_t position);

      void
      print_set(size_t i);

      void
      print_stats() const;

      void
      create_reductions();

      private:

      void
      create_all_items();

      void
      create_start_set();

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
        return m_set_symbols.emplace(item).first;
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

      SetSymbolHash m_set_symbols;
      SetTermLookaheadHash m_set_term_lookahead;
      ItemTreeHash m_item_tree;
      DistanceHash m_distance_hash;

      std::vector<bool> m_nullable;

      // follow and first sets
      FirstSet m_first_sets;
      FollowSet m_follow_sets;

      Items m_all_items;

      std::vector<std::vector<size_t>> m_item_membership;

      int m_lookahead_collisions = 0;
      int m_reuse = 0;

      auto
      insert_transition(const SetSymbolRules& tuple)
      -> decltype(m_set_symbols.emplace(tuple));
    };

    inline
    bool
    operator==(const ItemTreePointers& lhs, const ItemTreePointers& rhs)
    {
      return lhs.source == rhs.source;
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
  struct hash<earley::fast::SetSymbolRules>
  {
    size_t
    operator()(const earley::fast::SetSymbolRules& s)
    {
      return s.hash;
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
  struct hash<tuple<const earley::fast::Item*, size_t>>
  {
    size_t
    operator()(const tuple<const earley::fast::Item*, size_t>& p)
    {
      size_t result = std::hash<const earley::fast::Item*>()(std::get<0>(p));
      hash_combine(result, std::get<1>(p));
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
}

#endif
