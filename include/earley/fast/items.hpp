#ifndef EARLEY_FAST_ITEMS_HPP_INCLUDED
#define EARLEY_FAST_ITEMS_HPP_INCLUDED

#include "earley/fast/grammar.hpp"

namespace earley::fast
{
  class Item
  {
  };

  class Items
  {
    public:
    Items(const std::vector<RuleList>& rules);

    private:
    std::unordered_map<const Rule*, std::vector<Item>> m_item_map;
  };
}

#endif
