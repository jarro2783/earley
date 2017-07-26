#ifndef EARLEY_HPP_INCLUDED
#define EARLEY_HPP_INCLUDED

#include <iostream>

#include <functional>
#include <initializer_list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

template <typename T>
struct is_initializer_list;

template <typename T>
struct is_initializer_list
  : public std::bool_constant<false> {};

template <typename V>
struct is_initializer_list<std::initializer_list<V>>
  : public std::bool_constant<true> {};

template <typename T>
constexpr auto is_initializer_list_v = is_initializer_list<T>::value;

template <class T>
void
hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

namespace earley
{
  class Item;
}

namespace std
{
  template <>
  struct hash<earley::Item>
  {
    size_t
    operator()(const earley::Item&) const;
  };
}

namespace earley
{
  template <typename T>
  struct ActionType;

  template <typename T>
  struct ActionType<std::unordered_map<std::string, T(*)(std::vector<T>&)>>
  {
    typedef T type;
  };

  struct Epsilon {};

  typedef std::function<bool(char)> Scanner;

  inline
  Scanner
  scan_range(char begin, char end)
  {
    return [=](char c){
      return c >= begin && c <= end;
    };
  }

  inline
  Scanner
  scan_char(char c)
  {
    return [=](char cc) {
      return c == cc;
    };
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, const Scanner&)
  {
    os << "scanfn";
    return os;
  }

  // An entry is an epsilon, a pointer to another non-terminal, or
  // any number of ways of specifying a terminal
  typedef std::variant<
    Epsilon,
    size_t,
    std::function<bool(char)>
  > Entry;

  typedef std::tuple<std::string, std::vector<size_t>> ActionArgs;

  class Rule
  {
    public:

    Rule(size_t nonterminal, std::vector<Entry> entries, ActionArgs actions = {})
    : m_nonterminal(nonterminal)
    , m_entries(std::move(entries))
    , m_actions(std::move(actions))
    {
    }

    std::vector<Entry>::const_iterator
    begin() const
    {
      return m_entries.begin();
    }

    std::vector<Entry>::const_iterator
    end() const
    {
      return m_entries.end();
    }

    size_t
    nonterminal() const
    {
      return m_nonterminal;
    }

    const ActionArgs&
    actions() const
    {
      return m_actions;
    }

    private:

    size_t m_nonterminal;
    std::vector<Entry> m_entries;
    ActionArgs m_actions;
  };

  class Item
  {
    public:

    Item(const Rule& rule)
    : m_rule(&rule)
    , m_start(0)
    , m_current(rule.begin())
    {
    }

    Item(const Rule& rule, size_t start)
    : m_rule(&rule)
    , m_start(start)
    , m_current(rule.begin())
    {
    }

    Item(const Item&) = default;
    Item(Item&&) = default;
    Item& operator=(const Item&) = default;


    std::vector<Entry>::const_iterator
    position() const
    {
      return m_current;
    }

    std::vector<Entry>::const_iterator
    end() const
    {
      return m_rule->end();
    }

    Item
    next() const
    {
      Item incremented(*this);
      ++incremented.m_current;

      return incremented;
    }

    size_t
    where() const
    {
      return m_start;
    }

    Item
    start(size_t where) const
    {
      Item started(*this);
      started.m_start = where;
      return started;
    }

    size_t
    nonterminal() const
    {
      return m_rule->nonterminal();
    }

    const Rule&
    rule() const
    {
      return *m_rule;
    }

    std::ostream&
    print(std::ostream&, const std::unordered_map<size_t, std::string>&) const;

    private:

    friend size_t std::hash<Item>::operator()(const Item&) const;
    friend bool operator==(const Item&, const Item&);

    const Rule* m_rule;
    size_t m_start;
    std::vector<Entry>::const_iterator m_current;
  };

  typedef std::unordered_set<Item> ItemSet;
  typedef std::vector<ItemSet> ItemSetList;
  typedef std::vector<Rule> RuleList;

  inline
  bool
  operator==(const Item& lhs, const Item& rhs)
  {
    bool eq = lhs.m_start == rhs.m_start
      && lhs.m_rule == rhs.m_rule
      && lhs.m_current == rhs.m_current;

    return eq;
  }

  inline
  bool
  operator==(const Epsilon&, const Epsilon&)
  {
    return true;
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, const Item& item)
  {
    item.print(os, {});
    return os;
  }

  struct NtPrinter
  {
    NtPrinter(const std::unordered_map<size_t, std::string>& names,
      size_t id)
    : m_names(names)
    , m_id(id)
    {
    }

    void
    print(std::ostream& os) const
    {
      auto iter = m_names.find(m_id);

      if (iter != m_names.end())
      {
        os << iter->second;
      }
      else
      {
        os << m_id;
      }
    }

    private:
    const std::unordered_map<size_t, std::string>& m_names;
    size_t m_id;
  };

  inline
  std::ostream&
  operator<<(std::ostream& os, const NtPrinter& printer)
  {
    printer.print(os);
    return os;
  }

  inline
  NtPrinter
  print_nt(const std::unordered_map<size_t, std::string>& names, size_t id)
  {
    return NtPrinter(names, id);
  }

  inline
  std::ostream&
  Item::print(std::ostream& os,
    const std::unordered_map<size_t, std::string>& names
  ) const
  {
    auto& item = *this;

    os << print_nt(names, item.m_rule->nonterminal()) << " -> ";
    auto iter = item.m_rule->begin();

    while (iter != item.m_rule->end())
    {
      if (iter == item.m_current)
      {
        os << " ·";
      }

      auto& entry = *iter;

      if (std::holds_alternative<Epsilon>(entry))
      {
        os << u8" ε";
      }
      else if (std::holds_alternative<size_t>(entry))
      {
        os << " " << print_nt(names, std::get<size_t>(entry));
      }
      else if (std::holds_alternative<Scanner>(entry))
      {
        os << " '" << std::get<Scanner>(entry) << "'";
      }

      ++iter;
    }

    if (iter == item.m_current)
    {
      os << " ·";
    }

    os << " (" << item.m_start << ')';

    return os;
  }

  // A production is a single item that can produce something to be
  // parsed.
  // Empty, a non-terminal, or a single character
  typedef std::variant<
    Epsilon,
    std::string,
    std::function<bool(char)>,
    char
  > Production;

  typedef std::vector<Production> ProductionList;

  struct RuleWithAction
  {
    RuleWithAction(ProductionList production)
    : m_productions(std::move(production))
    {
    }

    RuleWithAction(ProductionList productions, ActionArgs args)
    : m_productions(std::move(productions))
    , m_args(std::move(args))
    {
    }

    const ProductionList&
    productions() const
    {
      return m_productions;
    }

    const ActionArgs&
    arguments() const
    {
      return m_args;
    }

    private:

    ProductionList m_productions;
    ActionArgs m_args;
  };

  class TreePointers
  {
    public:

    typedef std::map<size_t, std::pair<Item, size_t>> ItemLabels;
    typedef std::unordered_map<Item, ItemLabels> Pointers;
    typedef std::vector<Pointers> PointerList;

    void
    reduction(size_t where, size_t label, const Item& from, const Item& to)
    {
      insert(m_reductions, where, where, label, from, to);
    }

    void
    predecessor(size_t wherefrom, size_t whereto, size_t label,
      const Item& from, const Item& to)
    {
      insert(m_predecessors, wherefrom, whereto, label, from, to);
    }

    const PointerList&
    reductions() const
    {
      return m_reductions;
    }

    const PointerList&
    predecessors() const
    {
      return m_predecessors;
    }

    private:

    static
    void
    ensure_size(PointerList& p, size_t s)
    {
      if (p.size() < s + 1)
      {
        p.resize(s+1);
      }
    }

    void
    insert(
      PointerList& p,
      size_t wherefrom,
      size_t whereto,
      size_t label,
      const Item& from,
      const Item& to)
    {
      ensure_size(p, wherefrom);
      auto& pointers = p[wherefrom][from];
      pointers.insert({label, {to, whereto}});
    }

    PointerList m_reductions;
    PointerList m_predecessors;
  };

  // A grammar is a mapping from non-terminals to a list of rules
  // A rule is a list of productions
  typedef std::unordered_map<std::string, std::vector<RuleWithAction>>
    Grammar;

  ItemSetList
  invert_items(const ItemSetList& item_sets);

  // For each item set,
  //   indexed by rule id
  //     a sorted list of items
  std::vector<std::vector<std::vector<Item>>>
  sorted_index(const ItemSetList& item_sets);

  typedef std::vector<std::vector<std::vector<Item>>> SortedItemSets;
}

std::tuple<bool, double, earley::ItemSetList, earley::TreePointers>
process_input(
  bool debug,
  size_t start,
  const std::string& input,
  const std::vector<earley::RuleList>& rules,
  const std::unordered_map<std::string, size_t>& names = {}
);

std::tuple<std::vector<earley::RuleList>,
  std::unordered_map<std::string, size_t>>
generate_rules(earley::Grammar& grammar);

std::vector<bool>
find_nullable(const std::vector<earley::RuleList>& rules);

namespace std
{
  template <>
  struct hash<earley::Epsilon>
  {
    inline
    size_t
    operator()(const earley::Epsilon&) const
    {
      return 43;
    }
  };

  inline
  size_t
  hash<earley::Item>::operator()(const earley::Item& item) const
  {
    size_t result = item.m_start;
    hash_combine(result, &*item.m_current);
    hash_combine(result, item.m_rule);

    return result;
  }
}

namespace earley
{
  namespace values
  {
    struct Empty {};
    struct Failed {};
  }

  template <typename... T>
  using ActionResult = std::variant<
    values::Failed,
    values::Empty,
    char,
    T...
  >;

  namespace detail
  {
    struct ParserActions
    {
      ParserActions(const std::string& input)
      : m_input(input)
      {
      }

      bool
      seen(const Item& item) const
      {
        return m_visited.find(item) != m_visited.end();
      }

      void
      visit(const Item& item)
      {
        m_visited.insert(item);
      }

      void
      unvisit(const Item& item)
      {
        m_visited.erase(item);
      }

      public:
      template <typename Actions>
      auto
      do_actions(const Item& root, const Actions& actions,
        const SortedItemSets& item_sets)
      {
        visit(root);
        auto result = process_item(root, item_sets, actions, 0);
        unvisit(root);

        return result;
      }

      private:

      template <typename Actions>
      auto
      process_item(const Item& item, const SortedItemSets& item_sets,
        const Actions& actions,
        size_t position)
      -> decltype(std::declval<typename Actions::mapped_type>()({values::Failed()}))
      {
        //std::cout << "Processing " << item.nonterminal() << " at " << position << std::endl;
        using Ret = decltype(actions.find("")->second({values::Failed()}));
        std::vector<Ret> results;

        if (traverse_item(results, actions, item,
            item.rule().begin(), item_sets, position))
        {
          if (std::get<1>(item.rule().actions()).size() == 0)
          {
            return values::Empty();
          }

          // return the action run on all the parts
          auto& action_runner = item.rule().actions();
          auto iter = actions.find(std::get<0>(action_runner));
          if (iter != actions.end() && iter->second)
          {
            std::vector<Ret> run_actions;
            for (auto& handle: std::get<1>(action_runner))
            {
              //std::cout << "Processing " << item.nonterminal() << std::endl;
              //std::cout << "Pushing back " << handle << std::endl;
              //std::cout << "and results is size " << results.size() << std::endl;
              run_actions.push_back(results[handle]);
            }

            return iter->second(run_actions);
          }
          else
          {
            return values::Empty();
          }
        }
        else
        {
          return values::Failed();
        }
      }

      template <typename Actions, typename Result>
      bool
      traverse_item(std::vector<Result>& results,
        const Actions& actions,
        const Item& item, decltype(item.position()) iter,
        const SortedItemSets& item_sets, size_t position)
      {
        if (iter == item.end())
        {
          if (item.where() != position)
          {
            //std::cout << "Not at the end of the item" << std::endl;
            return false;
          }
          return true;
        }

        if (std::holds_alternative<Scanner>(*iter))
        {
          auto matcher = std::get<Scanner>(*iter);
          if (position != m_input.size() && matcher(m_input[position]))
          {
            results.push_back(m_input[position]);
            if (!traverse_item(results, actions, item, iter + 1,
                item_sets, position + 1))
            {
              results.pop_back();
              return false;
            }
            return true;
          }
          else
          {
            return false;
          }
        }
        else if (std::holds_alternative<size_t>(*iter))
        {
          auto to_search = std::get<size_t>(*iter);
          for (auto& child: item_sets[position][to_search])
          {
            if (!seen(child) && child.nonterminal() == to_search)
            {
              //std::cout << item.nonterminal() << ":" << child.nonterminal()
              //  << " -> " << child.where() << std::endl;
              visit(child);
              auto value = process_item(child, item_sets, actions, position);

              if (!std::holds_alternative<values::Failed>(value))
              {
                results.push_back(value);
                if (traverse_item(results, actions, item, iter + 1, item_sets,
                  child.where()))
                {
                  unvisit(child);
                  return true;
                }
                results.pop_back();
              }
              unvisit(child);
            }
          }
          return false;
        }

        return true;
      }

      private:
      const std::string& m_input;
      std::unordered_set<Item> m_visited;
    };

    struct ForestActions
    {
      ForestActions(const TreePointers& pointers, const std::string& input)
      : m_pointers(pointers)
      , m_input(input)
      {
      }

      // Run the actions for a complete item
      // Calls item_entry_action to run the entry
      // actions and runs the action on the resulting vector
      template <typename Actions>
      typename ActionType<Actions>::type
      item_action(const Actions& actions, const Item& item, size_t which)
      {
        //using Ret = decltype(actions.find("")->second({values::Failed()}));
        using Ret = typename ActionType<Actions>::type;
        std::vector<Ret> results;
        item_entry_action(actions, item, results, which);

        //run the actual actions on results
        auto& action_runner = item.rule().actions();
        auto iter = actions.find(std::get<0>(action_runner));
        if (iter != actions.end() && iter->second)
        {
          std::vector<Ret> run_actions;
          for (auto& handle: std::get<1>(action_runner))
          {
            //std::cout << "Processing " << item.nonterminal() << std::endl;
            //std::cout << "Pushing back " << handle << std::endl;
            //std::cout << "and results is size " << results.size() << std::endl;
            run_actions.push_back(results[handle]);
          }

          return iter->second(run_actions);
        }
        else
        {
          return values::Empty();
        }
      }

      // Recursively go through the predecessors of an item and
      // run the reduction for each entry.
      // A reduction is just processing a full action with
      // item_action.
      template <typename Actions, typename Results>
      void
      item_entry_action(const Actions& actions, const Item& item,
        Results& results,
        size_t which)
      {
        // visit the predecessor first
        // then our reduction

        std::cout << item << std::endl;
        // find the predecessor
        {
          auto predecessor = find_previous(
            m_pointers.predecessors(), which, item);

          if (predecessor)
          {
            item_entry_action(actions, predecessor->first,
              results, predecessor->second);
          }
        }

        // do our reduction
        auto reduction = find_previous(m_pointers.reductions(), which, item);

        if (reduction)
        {
          // this builds the current node
          results.push_back(
            item_action(actions, reduction->first, reduction->second));
        }
        else
        {
          //we are either at the start or this is a scan
          auto current = item.position();
          if (current == item.rule().begin())
          {
            std::cout << "Start" << std::endl;
            return;
          }

          --current;
          if (std::holds_alternative<Scanner>(*current))
          {
            std::cout << "Scanning " << m_input[which-1] << " at " << which << std::endl;
            results.push_back(m_input[which-1]);
          }
        }
      }

      private:

      const std::pair<Item, size_t>*
      find_previous(const TreePointers::PointerList& pointers,
        size_t which, const Item& item)
      {
        if (which >= pointers.size())
        {
          return nullptr;
        }

        auto iter = pointers[which].find(item);
        // if there is no predecessor we must be at the end
        if (iter != pointers[which].end())
        {
          auto& labels = iter->second;
          auto label = labels.rbegin();
          if (label != labels.rend())
          {
            return &label->second;
          }
        }

        return nullptr;
      }

      const TreePointers& m_pointers;
      const std::string& m_input;
    };

  }

  template <typename Actions>
  typename ActionType<Actions>::type
  run_actions(const TreePointers& pointers,
    size_t start,
    const std::string& input,
    const Actions& actions,
    const ItemSetList& item_sets)
  {

#if 0
    std::cout << "Inverted:" << std::endl;
    size_t n = 0;
    for (auto& items : inverted)
    {
      std::cout << "Position " << n << std::endl;
      for (auto& item : items)
      {
        std::cout << item << std::endl;
      }
      ++n;
    }
#endif

#if 0
    std::cout << "Sorted and indexed:" << std::endl;
    n = 0;
    for (auto& items: sorted)
    {
      std::cout << "Set " << n << std::endl;
      size_t rule = 0;
      for (auto& rules: items)
      {
        if (rules.size())
        {
          std::cout << "  Rule: " << rule << std::endl;
          for (auto& item: rules)
          {
            std::cout << "    " << item << std::endl;
          }
        }
        ++rule;
      }
      ++n;
    }
#endif

    // start searching from an item that was completed in
    // state zero and finishes at the length of the item sets
    detail::ParserActions do_actions(input);

    for (auto& item: item_sets[input.size()])
    {
      if (item.where() == 0 && item.nonterminal() == start)
      {
        detail::ForestActions run(pointers, input);
        return run.item_action(actions, item, input.size());
      }
    }

    return values::Failed();
  }
}

#endif
