#include "earley/fast/items.hpp"

namespace earley::fast
{

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

Items::Items(const std::vector<grammar::RuleList>& nonterminals,
  const grammar::FirstSets& firsts,
  const grammar::FollowSets& follows)
: m_firsts(firsts)
, m_follows(follows)
{
  for (auto& rules: nonterminals)
  {
    for (auto& rule: rules)
    {
      m_item_map[&rule];
    }
  }
}

const Item*
Items::get_item(const grammar::Rule* rule, int position)
{
  auto iter = m_item_map.find(rule);

  if (iter == m_item_map.end())
  {
    throw NoSuchItem();
  }

  auto length = rule->end() - rule->begin();

  if (position > length)
  {
    throw NoSuchItem();
  }

  ensure_size(iter->second, position);

  auto& ptr = iter->second[position];

  if (ptr == nullptr)
  {
    HashSet<int> lookahead;
    ptr = std::make_shared<Item>(rule, rule->begin()+position, lookahead);
  }

  return ptr.get();
}

}
