#ifndef EARLEY_HPP_INCLUDED
#define EARLEY_HPP_INCLUDED

#include <iostream>

#include <functional>
#include <initializer_list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "earley_hash_set.hpp"
#include "earley/variant.hpp"

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
  template <template <typename, typename, typename...> typename Map, typename K, typename V, typename... Args>
  Map<V, K>
  invert_map(const Map<K, V, Args...>& map)
  {
    Map<V, K> inverted;

    for (auto& p: map)
    {
      inverted[p.second] = p.first;
    }

    return inverted;
  }

  typedef variant<
    size_t,
    char
  > RawSymbol;

  struct Symbol
  {
    Symbol() = default;

    template <typename T,
      typename = typename
        std::enable_if<
          std::is_constructible<RawSymbol, T>::value
        >::type
    >
    Symbol(T&& s)
    : code(std::move(s))
    , empty(false)
    {
    }

    RawSymbol code;
    bool empty;

    bool
    terminal() const
    {
      return holds<char>(code);
    }
  };

  inline
  bool
  operator==(const Symbol& lhs, const Symbol& rhs)
  {
    return lhs.code == rhs.code;
  }

  inline
  bool
  operator!=(const Symbol& lhs, const Symbol& rhs)
  {
    return !operator==(lhs, rhs);
  }

  template <typename T>
  struct ActionType;

  template <typename T>
  struct ActionType<std::unordered_map<std::string, T(*)(std::vector<T>&)>>
  {
    typedef T type;
  };

  typedef std::function<bool(char)> ScanFn;

  class Scanner
  {
    public:
    Scanner(char c)
    : left(c)
    , right(-1)
    , m_scan([=](char cc) {
        return c == cc;
      })
    {
    }

    Scanner(char begin, char end)
    : left(begin)
    , right(end)
    , m_scan([=](char c){
        return c >= begin && c <= end;
      })
    {
    }

    auto
    operator()(char c) const
    {
      return m_scan(c);
    }

    void
    print(std::ostream& os) const
    {
      os << '[' << left;
      if (right != -1)
      {
        os << '-' << right;
      }
      os << ']';
    }

    const char left;
    const char right;

    private:
    ScanFn m_scan;
  };

  inline
  Scanner
  scan_range(char begin, char end)
  {
    return Scanner(begin, end);
  }

  inline
  Scanner
  scan_char(char c)
  {
    return Scanner(c);
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, const Scanner& s)
  {
    s.print(os);
    return os;
  }

  // An entry is a non-terminal, or
  // a scanner for a terminal
  typedef std::variant<
    size_t,
    Scanner
  > RawEntry;

  struct Entry
  {
    template <typename T>
    Entry(T&& t)
    : entry(std::forward<T>(t))
    {
    }

    bool
    terminal() const
    {
      return holds<Scanner>(entry);
    }

    bool
    empty() const
    {
      return m_empty;
    }

    void
    set_empty()
    {
      m_empty = true;
    }

    RawEntry entry;
    bool m_empty = false;
  };

  template <typename T>
  bool
  holds(const Entry& e)
  {
    return holds<T>(e.entry);
  }

  template <typename T>
  bool
  holds(Entry& e)
  {
    return holds<T>(e.entry);
  }

  template <typename T>
  decltype(auto)
  get(const Entry& e)
  {
    return get<T>(e.entry);
  }

  template <typename T>
  auto
  visit(T&& t, const Entry& e)
  {
    return std::visit(std::forward<T>(t), e.entry);
  }

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

    bool empty() const
    {
      return m_entries.empty();
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

    auto
    dot() const
    {
      return this->position();
    }

    auto
    dot_index() const
    {
      return this->position() - m_rule->begin();
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

      // TODO: this is a bit of a hack
      incremented.m_lookahead.clear();

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

    void
    add_lookahead(int symbol)
    {
      m_lookahead.insert(symbol);
    }

    bool
    in_lookahead(int symbol) const
    {
      return m_lookahead.count(symbol);
    }

    bool empty() const
    {
      return m_current == m_rule->end();
    }

    std::ostream&
    print(std::ostream&, const std::unordered_map<size_t, std::string>&) const;

    private:

    friend size_t std::hash<Item>::operator()(const Item&) const;
    friend bool operator==(const Item&, const Item&);

    const Rule* m_rule;
    size_t m_start;
    std::vector<Entry>::const_iterator m_current;
    std::unordered_set<int> m_lookahead;
  };

  inline
  bool
  operator<(const std::pair<Item, size_t>& lhs,
    const std::pair<Item, size_t>& rhs)
  {
    //ignore the label because I think it's redundant
    auto& li = lhs.first;
    auto& ri = rhs.first;

    if (&li.rule() < &ri.rule())
    {
      return true;
    }
    else if (&li.rule() > &ri.rule())
    {
      return false;
    }

    if (li.where() < ri.where())
    {
      return true;
    }
    else if (li.where() > ri.where())
    {
    }

    auto left_length = li.rule().end() - li.rule().begin();
    auto right_length = ri.rule().end() - ri.rule().begin();

    if (left_length < right_length)
    {
      return true;
    }
    else if (left_length > right_length)
    {
      return false;
    }

    return false;
  }

  //typedef std::unordered_set<Item> ItemSet;
  typedef std::tuple<Item, size_t> TransitiveItem;
  typedef HashSet<Item> ItemSet;
  typedef HashSet<TransitiveItem> TransitiveItemSet;
  typedef std::vector<ItemSet> ItemSetList;
  typedef std::vector<TransitiveItemSet> TransitiveItemSetList;
  typedef std::vector<Rule> RuleList;

  class ParseGrammar
  {
    public:

    ParseGrammar(size_t start, std::vector<RuleList> rules)
    : m_start(start)
    , m_rules(std::move(rules))
    {
    }

    size_t
    start() const
    {
      return m_start;
    }

    const std::vector<RuleList>&
    rules() const
    {
      return m_rules;
    }

    auto&
    get(size_t rule) const
    {
      return m_rules[rule];
    }

    private:
    size_t m_start;
    std::vector<RuleList> m_rules;
  };

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

      if (holds<size_t>(entry))
      {
        os << " " << print_nt(names, get<size_t>(entry));
      }
      else if (holds<Scanner>(entry))
      {
        os << " '" << get<Scanner>(entry) << "'";
      }

      ++iter;
    }

    if (iter == item.m_current)
    {
      os << " ·";
    }

    os << " (" << item.m_start << ") : (";

    for (auto symbol: m_lookahead)
    {
      os << symbol << " ";
    }

    os << ")";

    return os;
  }

  inline
  void
  print(
    std::ostream& os,
    const TransitiveItem& t,
    const std::unordered_map<size_t, std::string>& names
  )
  {
    os << names.find(get<1>(t))->second << ": ";
    get<0>(t).print(os, names);
    os << std::endl;
  }

  // A production is a single item that can produce something to be
  // parsed.
  // A non-terminal, or a single character
  typedef std::variant<
    std::string,
    Scanner,
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

    // TODO: this is badly named
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

    TreePointers() = default;
    TreePointers(const TreePointers&) = default;
    TreePointers(TreePointers&&) = default;

    typedef std::map<size_t, std::set<std::pair<Item, size_t>>> ItemLabels;
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

      //if (pointers[label].size() != 0)
      //{
      //  std::cout << "Duplicate at " << from << ": " << wherefrom << std::endl;
      //}

      pointers[label].insert({to, whereto});
    }

    PointerList m_reductions;
    PointerList m_predecessors;
  };

  // A grammar is a mapping from non-terminals to a list of rules
  // A rule is a list of productions
  typedef std::unordered_map<std::string, std::vector<RuleWithAction>>
    Grammar;

  typedef std::unordered_map<std::string, size_t> TerminalMap;

  ItemSetList
  invert_items(const ItemSetList& item_sets);

  // For each item set,
  //   indexed by rule id
  //     a sorted list of items
  std::vector<std::vector<std::vector<Item>>>
  sorted_index(const ItemSetList& item_sets);

  typedef std::vector<std::vector<std::vector<Item>>> SortedItemSets;

  template <typename T>
  void
  add_action(
    const std::string& name,
    std::unordered_map<std::string, T (*)(std::vector<T>&)>& actions,
    T (*fun)(std::vector<T>&)
  )
  {
    actions[name] = fun;
  }

  template <typename Result>
  Result
  handle_pass(std::vector<Result>& parts)
  {
    return parts.at(0);
  }

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
generate_rules(const earley::Grammar& grammar);

std::vector<bool>
find_nullable(const std::vector<earley::RuleList>& rules);

namespace std
{
  inline
  size_t
  hash<earley::Item>::operator()(const earley::Item& item) const
  {
    size_t result = item.m_start;

    if (item.empty())
    {
      hash_combine(result, 0);
    }
    else
    {
      hash_combine(result, &*item.m_current);
    }
    hash_combine(result, item.m_rule);

    return result;
  }

  template <>
  struct hash<tuple<earley::Item, size_t>>
  {
    size_t
    operator()(const tuple<earley::Item, size_t>& t)
    {
      size_t result = get<1>(t);
      hash<earley::Item> hi;
      hash_combine(result, hi(get<0>(t)));
      return result;
    }
  };

  template <>
  struct hash<earley::RawSymbol>
  {
    size_t
    operator()(const earley::RawSymbol& s)
    {
      if (earley::holds<size_t>(s))
      {
        return earley::get<size_t>(s);
      }
      else
      {
        return earley::get<char>(s);
      }
    }
  };

  template <>
  struct hash<earley::Symbol>
  {
    size_t
    operator()(const earley::Symbol& s)
    {
      size_t result = hash<earley::RawSymbol>()(s.code);

      return result;
    }
  };
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
          if (get<1>(item.rule().actions()).size() == 0)
          {
            return values::Empty();
          }

          // return the action run on all the parts
          auto& action_runner = item.rule().actions();
          auto iter = actions.find(get<0>(action_runner));
          if (iter != actions.end() && iter->second)
          {
            std::vector<Ret> run_actions;
            for (auto& handle: get<1>(action_runner))
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

        if (holds<Scanner>(*iter))
        {
          auto matcher = get<Scanner>(*iter);
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
        else if (holds<size_t>(*iter))
        {
          auto to_search = get<size_t>(*iter);
          for (auto& child: item_sets[position][to_search])
          {
            if (!seen(child) && child.nonterminal() == to_search)
            {
              //std::cout << item.nonterminal() << ":" << child.nonterminal()
              //  << " -> " << child.where() << std::endl;
              visit(child);
              auto value = process_item(child, item_sets, actions, position);

              if (!holds<values::Failed>(value))
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
      ForestActions(
        const TreePointers& pointers,
        const std::string& input,
        [[maybe_unused]] const std::unordered_map<size_t, std::string>& names
      )
      : m_pointers(pointers)
      , m_input(input)
      //, m_names(names)
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
        auto iter = actions.find(get<0>(action_runner));
        if (iter != actions.end() && iter->second)
        {
          std::vector<Ret> run_actions;
          for (auto& handle: get<1>(action_runner))
          {
            run_actions.push_back(results.at(handle));
          }

          //std::cout << "Tree: |";
          //for (size_t i = 0; i != which; ++i)
          //{
          //  std::cout << ' ';
          //}
          //item.print(std::cout, m_names);
          //std::cout << std::endl;

          return iter->second(run_actions);
        }
        else
        {
          //std::cout << "Tree: |";
          //for (size_t i = 0; i != which; ++i)
          //{
          //  std::cout << ' ';
          //}
          //item.print(std::cout, m_names);
          //std::cout << std::endl;
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

        //std::cout << item << std::endl;
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
          //we are either at the start, this is a scan, or the item is nullable
          auto current = item.position();
          if (current == item.rule().begin())
          {
            // Do nothing for the start of the chain
            return;
          }

          --current;
          if (holds<Scanner>(*current))
          {
            results.push_back(m_input[which-1]);
          }
          else
          {
            // let's just assume that it must be nullable for now
            // we should probably check this
            results.push_back(values::Empty());
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
          if (label != labels.rend() && label->second.begin() != label->second.end())
          {
            return &*label->second.begin();
          }
        }

        return nullptr;
      }

      const TreePointers& m_pointers;
      const std::string& m_input;
      //const std::unordered_map<size_t, std::string>& m_names;
    };
  }

  template <typename Actions>
  typename ActionType<Actions>::type
  run_actions(const TreePointers& pointers,
    size_t start,
    const std::string& input,
    const Actions& actions,
    const ItemSetList& item_sets,
    const std::unordered_map<std::string, size_t>& names
  )
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
      if (item.where() == 0 && item.nonterminal() == start &&
        item.position() == item.rule().end())
      {
        auto inverted = invert_map(names);
        detail::ForestActions run(pointers, input, inverted);
        return run.item_action(actions, item, input.size());
      }
    }

    return values::Failed();
  }
}

#endif
