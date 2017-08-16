#include "earley/fast.hpp"

namespace earley::fast
{

Symbol
create_token(char c)
{
  return Symbol{c, false};
}

void
ItemSet::add_start_item(const Item* item, size_t distance)
{
  m_core->add_start_item(item);
  m_distances.push_back(distance);
}

Parser::Parser(const ParseGrammar& grammar)
: m_grammar(grammar)
{
  create_start_set();
}

void
Parser::parse(const std::string& input)
{
  size_t position = 0;

  create_start_set();

  while (position < input.size())
  {
    auto set = create_new_set(position, input[position]);

    // if this is a new set, then expand it
    auto result = m_item_set_hash.insert(set);

    if (result.second)
    {
      expand_set(set.get());
    }

    ++position;
  }
}

void
Parser::create_start_set()
{
  ItemSet* items = nullptr;
  for (auto& rule: m_grammar.rules()[m_grammar.start()])
  {
    auto item = get_item(&rule, 0);
    items->add_start_item(item, 0);
  }

  expand_set(items);
}

void
Parser::expand_set(ItemSet* items)
{
  add_empty_symbol_items(items);
  add_non_start_items(items);
}

const Item*
Parser::get_item(const earley::Rule*, size_t) const
{
  // TODO: implement this with the right rule
  //return &m_items.find(rule)->second[dot];
  return nullptr;
}

const Item*
Parser::get_item(const Rule*, size_t) const
{
  // TODO: implement this with the right rule
  //return &m_items.find(rule)->second[dot];
  return nullptr;
}

void
Parser::add_empty_symbol_items(ItemSet* items)
{
  auto core = items->core();
  // for each item in the start items, add the next items
  // as long as the right hand sides can derive empty
  // TODO: nullable && generate next item
  for (size_t i = 0; i != core->start_items(); ++i)
  {
    auto item = core->item(i);
    for (
      auto pos = item->dot();
      pos != item->rule()->end(); // && symbol empty
      ++pos)
    {
      items->add_derived_item(
        get_item(item->rule(), (pos + 1) - item->rule()->begin()),
        i);
    }
  }
}

void
Parser::add_non_start_items(ItemSet* items)
{
  auto core = items->core();
  for (size_t i = 0; i < core->all_items(); ++i)
  {
    item_transition(items, core->item(i), i);
  }
}

// If this item has a symbol after the dot, add an index for
// (current item set, item->symbol) -> item
void
Parser::item_transition(ItemSet* items, const Item* item, size_t index)
{
  auto rule = item->rule();
  auto core = items->core();

  if (item->dot() != rule->end())
  {
    auto& symbol = *item->dot();
    SetSymbolRules tuple{items->core(), symbol, {}};
    auto iter = m_set_symbols.find(tuple);

    if (iter == m_set_symbols.end())
    {
      //insert a new set
      iter = new_symbol_index(tuple);
      if (!symbol.terminal())
      {
        (void)core;
        // prediction
        // insert initial items for this symbol
        for (auto& prediction: m_grammar.get(get<size_t>(symbol.code)))
        {
          add_initial_item(core, get_item(&prediction, 0));
        }
      }
    }

    // TODO: add this item to the (set, symbol) index
    iter->transitions.push_back(index);

    // if this symbol can derive empty then add the next item too
    if (symbol.empty)
    {
      // nullable completion
      add_initial_item(core, get_item(rule, rule->end() - item->dot()+1));
    }

    // TODO: what are the reduce vectors?
  }
}

void
Parser::add_initial_item(ItemSetCore* core, const Item* item)
{
  // add the item if it doesn't already exist
  for (size_t i = core->start_items(); i != core->all_items(); ++i)
  {
    if (item == core->item(i))
    {
      return;
    }
  }

  core->add_derived_item(item);
}

// Do scans and completions to start the current set
// find it in the hash table, then expand it if it's new
std::shared_ptr<ItemSet>
Parser::create_new_set(size_t position, char input)
{
  Symbol token = create_token(input);
  auto core = std::make_shared<ItemSetCore>();
  auto current_set = std::make_shared<ItemSet>(core.get());

  auto& previous_set = m_itemSets[position];
  auto& previous_core = *previous_set.core();

  // look up the symbol index for the previous set
  auto scans = m_set_symbols.find(SetSymbolRules{
    &previous_core, token, {}});

  if (scans != m_set_symbols.end())
  {
    // do all the scans
    for (auto transition: scans->transitions)
    {
      // TODO: lookahead
      current_set->add_start_item(previous_core.item(transition),
        previous_set.distance(transition) + 1);
    }

    // now do all the completed items
    for (size_t i = 0; i < core->start_items(); ++i)
    {
      auto item = core->item(i);
      auto rule = item->rule();
      // TODO: change this to empty tail
      if (item->dot() == rule->end())
      {
        auto from = position - current_set->distance(i);
        auto from_core = m_itemSets[from].core();

        // find the symbol for the lhs of this rule in set that predicted this
        // i.e., this is a completion: find the items it completes
        auto transitions = m_set_symbols.find(SetSymbolRules{
          from_core, Symbol(), {}
        });

        for (auto transition: transitions->transitions)
        {
          auto* item = from_core->item(transition);
          auto* next = get_item(item->rule(), item->dot() - item->rule()->begin() + 1);
          current_set->add_start_item(next, 0);
        }
      }
    }
  }

  return current_set;
}

}
