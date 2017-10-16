#include "earley/fast/items.hpp"

namespace earley::fast
{

namespace
{

template <typename Iterator>
bool
empty_sequence(const std::vector<bool>& nullable,
  Iterator begin,
  Iterator end
)
{
  Iterator current = begin;
  while (current != end)
  {
    if (current->terminal)
    {
      return false;
    }

    if (!nullable[current->index])
    {
      return false;
    }
    ++current;
  }
  return true;
}

struct RuleItemComparator
{
  bool
  operator()(
    const grammar::Rule* r,
    const std::pair<const grammar::Rule*, ItemStore>& p
  )
  {
    return r < p.first;
  }

  bool
  operator()(
    const std::pair<const grammar::Rule*, ItemStore>& p,
    const grammar::Rule* r
  )
  {
    return p.first < r;
  }
};

}

// TODO: move this to utils and test it
template <typename T>
void
ensure_size(T& t, size_t size)
{
  if (size >= t.size())
  {
    t.resize(size+1);
  }
}

void
Items::fill_to(const grammar::Rule* rule, ItemStore& items, size_t position)
{
  items.reserve(position);
  for (size_t i = items.size(); i <= position; ++i)
  {
    auto lookahead = sequence_lookahead(*rule, rule->begin() + i,
      m_firsts, m_follows);

    items.emplace_back(
      rule,
      rule->begin()+i,
      std::move(lookahead),
      empty_sequence(m_nullable,
        rule->begin()+i,
        rule->end()
      ),
      m_item_index
    );
    ++m_item_index;
  }
}

Items::Items(const std::vector<grammar::RuleList>& nonterminals,
  const grammar::FirstSets& firsts,
  const grammar::FollowSets& follows,
  const std::vector<bool>& nullable)
: m_firsts(firsts)
, m_follows(follows)
, m_nullable(nullable)
{
  for (auto& rules: nonterminals)
  {
    for (auto& rule: rules)
    {
      auto& position = insert_rule(&rule);
      fill_to(&rule, position, rule.end() - rule.begin());
    }
  }
}

// There better be no duplicates
ItemStore&
Items::insert_rule(const grammar::Rule* rule)
{
  auto pos = std::upper_bound(m_rule_map.begin(), m_rule_map.end(), rule,
    RuleItemComparator());

  auto result = m_rule_map.insert(pos, {rule, ItemStore()});

  return result->second;
}

const ItemStore*
Items::find_rule(const grammar::Rule* rule)
{
  auto pos = std::lower_bound(m_rule_map.begin(), m_rule_map.end(), rule,
    RuleItemComparator());

  if (pos == m_rule_map.end())
  {
    return nullptr;
  }

  if (pos->first != rule)
  {
    return nullptr;
  }

  return &pos->second;
}

std::ostream&
Item::print(
  std::ostream& os,
  const std::unordered_map<size_t, std::string>& names
) const
{
  auto& item = *this;

  os << print_nt(names, item.m_rule->nonterminal()) << " -> ";
  auto iter = item.m_rule->begin();

  while (iter != item.m_rule->end())
  {
    if (iter == item.m_position)
    {
      os << " ·";
    }

    auto& entry = *iter;

    if (entry.terminal)
    {
      os << " '" << entry.index << "'";
    }
    else
    {
      os << " " << print_nt(names, entry.index) << " ";
    }

    ++iter;
  }

  if (iter == item.m_position)
  {
    os << " ·";
  }

  os << ": ( ";
  for (auto symbol: m_lookahead)
  {
    os << symbol << " ";
  }
  os << ")";

  return os;
}

}
