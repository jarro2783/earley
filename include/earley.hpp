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

  // An entry is an epsilon, a pointer to another non-terminal, or
  // any number of ways of specifying a terminal
  typedef std::variant<
    Epsilon,
    size_t,
    char
  > Entry;

  class Item
  {
    public:

    Item(size_t nonterminal, std::initializer_list<Entry> list)
    : m_nonterminal(nonterminal)
    , m_entries(std::move(list))
    , m_start(0)
    , m_current(m_entries.begin())
    {
    }

    Item(size_t nonterminal, std::vector<Entry> list)
    : m_nonterminal(nonterminal)
    , m_entries(std::move(list))
    , m_start(0)
    , m_current(m_entries.begin())
    {
    }

    #if 0
    template <typename... Args,
      typename = typename std::enable_if<
        std::negation_v<
          std::disjunction<is_initializer_list<std::decay_t<Args>>...>
        >
      >::type
    >
    Item(Args&&... args)
    : Item({args...})
    {
    }
    #endif

    Item(const Item& rhs)
    : m_nonterminal(rhs.m_nonterminal)
    , m_entries(rhs.m_entries)
    , m_start(rhs.m_start)
    , m_current(m_entries.begin() + (rhs.m_current - rhs.m_entries.begin()))
    {
    }

    Item(size_t nonterminal)
    : Item(nonterminal, {Epsilon()})
    {
    }

    std::vector<Entry>::const_iterator
    position() const
    {
      return m_current;
    }

    std::vector<Entry>::const_iterator
    end() const
    {
      return m_entries.end();
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
      return m_nonterminal;
    }

    private:

    friend size_t std::hash<Item>::operator()(const Item&) const;
    friend bool operator==(const Item&, const Item&);
    friend std::ostream& operator<<(std::ostream&, const Item&);

    size_t m_nonterminal;
    std::vector<Entry> m_entries;
    size_t m_start;
    std::vector<Entry>::const_iterator m_current;
  };

  typedef std::unordered_set<Item> ItemSet;
  typedef std::vector<ItemSet> ItemSetList;

  inline
  bool
  operator==(const Item& lhs, const Item& rhs)
  {
    bool eq = lhs.m_start == rhs.m_start
      && lhs.m_entries == rhs.m_entries
      && (
          (lhs.m_current - lhs.m_entries.begin()) == 
          (rhs.m_current - rhs.m_entries.begin())
         )
      && lhs.m_nonterminal == rhs.m_nonterminal;

    //std::cout << lhs << " == " << rhs << ": " << eq << std::endl;

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
    os << item.m_nonterminal << " -> ";
    auto iter = item.m_entries.begin();

    while (iter != item.m_entries.end())
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
      else if (std::holds_alternative<char>(entry))
      {
        os << " '" << std::get<char>(entry) << "'";
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

  typedef std::variant<
    Epsilon,
    std::string,
    char
  > Production;

  typedef std::unordered_map<std::string, std::vector<std::vector<Production>>>
    Grammar;
}

void
process_input(
  size_t start,
  const std::string& input,
  const std::unordered_map<size_t, earley::ItemSet>& rules
);

std::tuple<std::unordered_map<size_t, earley::ItemSet>,
  std::unordered_map<std::string, size_t>>
generate_rules(earley::Grammar& grammar);

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
    hash_combine(result, (item.m_current - item.m_entries.begin()));

    for (auto& entry : item.m_entries)
    {
      hash_combine(result, entry);
    }

    hash_combine(result, item.m_nonterminal);

    //std::cout << "Hash: " << item << ": " << result << std::endl;

    return result;
  }
}
