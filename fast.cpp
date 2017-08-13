#include <memory>
#include <vector>

#include "earley.hpp"
#include "earley_hash_set.hpp"

namespace earley
{
  namespace fast
  {
    class Item
    {
      private:
      Rule* m_rule;
      std::vector<Entry>::const_iterator m_dot;
    };

    class ItemSetCore
    {
    };

    class ItemSet
    {
    };

    class Parser
    {
      private:
      std::vector<ItemSet> m_itemSets;
      std::vector<std::vector<Item>> m_items;
    };
  }
}
