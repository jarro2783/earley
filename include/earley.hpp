#include <functional>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "earley_hash_set.hpp"

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

  class Rule
  {
    public:

    Rule(size_t nonterminal, std::vector<Entry> entries)
    : m_nonterminal(nonterminal)
    , m_entries(std::move(entries))
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

    private:

    size_t m_nonterminal;
    std::vector<Entry> m_entries;
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

    private:

    friend size_t std::hash<Item>::operator()(const Item&) const;
    friend bool operator==(const Item&, const Item&);
    friend std::ostream& operator<<(std::ostream&, const Item&);

    const Rule& m_rule;
    size_t m_start;
    std::vector<Entry>::const_iterator m_current;
  };

  //typedef std::unordered_set<Item> ItemSet;
  typedef HashSet<Item> ItemSet;
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

  // A grammar is a mapping from non-terminals to a list of rules
  // A rule is a list of productions
  typedef std::unordered_map<std::string, std::vector<std::vector<Production>>>
    Grammar;
}

std::tuple<bool, double>
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
