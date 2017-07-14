#include <functional>
#include <initializer_list>
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

  typedef std::vector<int> ActionArgs;

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

    const std::vector<int>&
    actions() const
    {
      return m_actions;
    }

    private:

    size_t m_nonterminal;
    std::vector<Entry> m_entries;
    std::vector<int> m_actions;
  };

  class Item
  {
    public:

    Item(const Rule& rule)
    : m_rule(rule)
    , m_start(0)
    , m_current(rule.begin())
    {
    }

    Item(const Rule& rule, size_t start)
    : m_rule(rule)
    , m_start(start)
    , m_current(rule.begin())
    {
    }

    Item(const Item& rhs) = default;

    std::vector<Entry>::const_iterator
    position() const
    {
      return m_current;
    }

    std::vector<Entry>::const_iterator
    end() const
    {
      return m_rule.end();
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
      return m_rule.nonterminal();
    }

    const Rule&
    rule() const
    {
      return m_rule;
    }

    private:

    friend size_t std::hash<Item>::operator()(const Item&) const;
    friend bool operator==(const Item&, const Item&);
    friend std::ostream& operator<<(std::ostream&, const Item&);

    const Rule& m_rule;
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
      && &lhs.m_rule == &rhs.m_rule
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
    os << item.m_rule.nonterminal() << " -> ";
    auto iter = item.m_rule.begin();

    while (iter != item.m_rule.end())
    {
      if (iter == item.m_current)
      {
        os << " *";
      }

      auto& entry = *iter;

      if (std::holds_alternative<Epsilon>(entry))
      {
        os << u8" ε";
      }
      else if (std::holds_alternative<size_t>(entry))
      {
        os << " " << std::get<size_t>(entry);
      }
      else if (std::holds_alternative<Scanner>(entry))
      {
        os << " '" << std::get<Scanner>(entry) << "'";
      }

      ++iter;
    }

    if (iter == item.m_current)
    {
      os << " *";
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

  // A grammar is a mapping from non-terminals to a list of rules
  // A rule is a list of productions
  typedef std::unordered_map<std::string, std::vector<RuleWithAction>>
    Grammar;

  ItemSetList
  invert_items(const ItemSetList& item_sets);
}

std::tuple<bool, double, earley::ItemSetList>
process_input(
  bool debug,
  size_t start,
  const std::string& input,
  const std::vector<earley::RuleList>& rules
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
    hash_combine(result, &item.m_rule);

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

  template <typename T>
  using ActionResult = std::variant<
    values::Failed,
    values::Empty,
    char,
    T
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
      do_actions(const Item& root, const std::vector<Actions>& actions,
        const ItemSetList& item_sets)
      {
        visit(root);
        auto result = process_item(root, item_sets, actions, 0);
        unvisit(root);

        return result;
      }

      private:

      template <typename Actions>
      auto
      process_item(const Item& item, const ItemSetList& item_sets,
        const std::vector<Actions>& actions,
        size_t position)
      -> decltype(actions[0]({values::Failed()}))
      {
        std::cout << "Processing " << item.nonterminal() << " at " << position << std::endl;
        using Ret = decltype(actions[0]({values::Failed()}));
        std::vector<Ret> results;

        if (traverse_item(results, actions, item,
            item.rule().begin(), item_sets, position))
        {
          if (item.rule().actions().size() == 0)
          {
            return values::Empty();
          }

          // return the action run on all the parts
          if (actions[item.nonterminal()])
          {
            std::vector<Ret> run_actions;
            for (auto& handle: item.rule().actions())
            {
              std::cout << "Processing " << item.nonterminal() << std::endl;
              std::cout << "Pushing back " << handle << std::endl;
              std::cout << "and results is size " << results.size() << std::endl;
              run_actions.push_back(results[handle]);
            }

            return actions[item.nonterminal()](run_actions);
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

      template <typename Action, typename Result>
      bool
      traverse_item(std::vector<Result>& results,
        const std::vector<Action>& actions,
        const Item& item, decltype(item.position()) iter,
        const ItemSetList& item_sets, size_t position)
      {
        if (iter == item.end())
        {
          return true;
        }

        if (std::holds_alternative<Scanner>(*iter))
        {
          auto matcher = std::get<Scanner>(*iter);
          if (position != m_input.size() && matcher(m_input[position]))
          {
            results.push_back(m_input[position]);
            if (!traverse_item(results, actions, item, iter + 1, item_sets, position + 1))
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
          for (auto& child: item_sets[position])
          {
            if (!seen(child) && child.nonterminal() == to_search)
            {
              std::cout << item.nonterminal() << ":" << child.nonterminal()
                << " -> " << child.where() << std::endl;
              visit(child);
              last = child.where();
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

      //find items that match `validate`
      template <typename Actions>
      auto
      item_search(const Item& root, const std::vector<Actions>& actions,
        const ItemSetList& item_sets, size_t position)
      -> decltype(actions[0]({values::Failed()}))
      {
        using Ret = decltype(actions[0]({values::Failed()}));
        std::vector<Ret> results;

        std::cout << "Looking for " << root.nonterminal() << " at "
          << position << std::endl;

        for (auto& entry: root.rule())
        {
          if (std::holds_alternative<Scanner>(entry))
          {
            auto matcher = std::get<Scanner>(entry);
            if (matcher(m_input[position]))
            {
              std::cout << "Matched " << m_input[position] << " at " << position << std::endl;
              results.push_back(m_input[position]);
              ++position;
            }
            else
            {
              // abort because it wasn't actually a match
              return values::Failed();
            }
          }
          else if (std::holds_alternative<size_t>(entry))
          {
            auto nt = std::get<size_t>(entry);
            bool found = false;
            // look for nonterminal starting at `position`
            for (auto& item: item_sets[position])
            {
              if (item.nonterminal() == nt && !seen(item))
              {
                visit(item);
                auto result = item_search(item, actions, item_sets, position);

                if (std::holds_alternative<values::Failed>(result))
                {
                  continue;
                }
                else
                {
                  results.push_back(result);
                }

                unvisit(item);
                position = item.where();
                found = true;
                break;
              }
            }

            if (!found)
            {
              std::cout << "Couldn't find " << nt << std::endl;
              results.push_back(values::Empty());
            }
          }
        }

        if (root.rule().actions().size() == 0)
        {
          return values::Empty();
        }

        // if we haven't aborted by now we need to return the action
        // run on all the parts
        std::vector<ActionResult<int>> run_actions;
        for (auto& handle: root.rule().actions())
        {
          std::cout << "Processing " << root.nonterminal() << std::endl;
          std::cout << "Pushing back " << handle << std::endl;
          std::cout << "and results is size " << results.size() << std::endl;
          run_actions.push_back(results[handle]);
        }

        if (actions[root.nonterminal()])
        {
          return actions[root.nonterminal()](run_actions);
        }
        else
        {
          return values::Empty();
        }
      }

      private:
      const std::string& m_input;
      std::unordered_set<Item> m_visited;
    };
  }

  template <typename Actions>
  void
  run_actions(size_t start, const std::string& input,
    const std::vector<Actions>& actions, const ItemSetList& item_sets)
  {
    auto inverted = invert_items(item_sets);

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

    // start searching from an item that was completed in
    // state zero and finishes at the length of the item sets
    detail::ParserActions do_actions(input);

    for (auto& item: inverted[0])
    {
      if (item.where() == input.size() && item.nonterminal() == start)
      {
        do_actions.do_actions(item, actions, inverted);
        return;
      }
    }
  }
}
