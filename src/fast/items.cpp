#include "earley/fast/items.hpp"

namespace earley::fast
{

Items::Items(const std::vector<RuleList>& nonterminals)
{
  for (auto& rules: nonterminals)
  {
    for (auto& rule: rules)
    {
      m_item_map[&rule];
    }
  }
}

}
