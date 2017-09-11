#include <cassert>

#include "earley/fast.hpp"
#include "earley/grammar_util.hpp"

namespace earley::fast
{

#ifdef NEW_GRAMMAR
grammar::Symbol
create_token(char c)
{
  return grammar::Symbol{c, true};
}
#else
Symbol
create_token(char c)
{
  return Symbol{c};
}
#endif

void
ItemSet::add_start_item(const PItem* item, size_t distance)
{
  hash_combine(m_hash, std::hash<const PItem*>()(item));
  hash_combine(m_hash, distance);

  m_core->add_start_item(item);
  m_distances.push_back(distance);
}

Parser::Parser(const grammar::Grammar& grammar_new)
: m_grammar_new(grammar_new)
, m_set_symbols(20000)
, m_all_items(m_grammar_new.all_rules(),
    m_grammar_new.first_sets(),
    m_grammar_new.follow_sets())
{
  create_start_set();
}

void
Parser::parse_input(const TerminalList& input)
{
  size_t position = 0;
  while (position < input.size())
  {
    parse(input, position);
    ++position;
  }
}

void
Parser::parse(const TerminalList& input, size_t position)
{
  auto set = create_new_set(position, input);

  // if this is a new set, then expand it
  auto result = m_item_set_hash.insert(set);

  if (result.second)
  {
    expand_set(set.get());
  }
  else
  {
    //std::cout << "Reused a set at position " << position << std::endl;
    //result.first->get()->print(m_names);
  }

  m_itemSets.push_back(result.first->get());
}

void
Parser::create_start_set()
{
  auto core = std::make_unique<ItemSetCore>();
  auto items = std::make_shared<ItemSet>(core.get());

#ifdef NEW_GRAMMAR
  for (auto& rule: m_grammar_new.rules(m_grammar_new.start()))
#else
  for (auto& rule: m_grammar.rules()[m_grammar.start()])
#endif
  {
    auto item = get_item(&rule, 0);
    items->add_start_item(item, 0);
  }

  expand_set(items.get());

  core.release();
  m_itemSets.push_back(items);
}

void
Parser::expand_set(ItemSet* items)
{
  add_empty_symbol_items(items);
  add_non_start_items(items);
}

const Item*
Parser::get_item(const grammar::Rule* rule, int dot)
{
  return m_all_items.get_item(rule, dot);
}

const earley::Item*
Parser::get_item(const earley::Rule* rule, size_t dot) const
{
  auto iter = m_items.find(rule);
  assert(iter != m_items.end());
  return &iter->second.at(dot);
}

void
Parser::add_empty_symbol_items(ItemSet* items)
{
  auto core = items->core();

  // for each item in the start items, add the next items
  // as long as the right hand sides can derive empty
  for (size_t i = 0; i != core->start_items(); ++i)
  {
    auto item = core->item(i);
    for (
      auto pos = item->dot();
      pos != item->rule().end() && nullable(*pos);
      ++pos)
    {
      items->add_derived_item(
        get_item(&item->rule(), (pos + 1) - item->rule().begin()),
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

  for (size_t i = 0; i < core->all_items(); ++i)
  {
    auto item = core->item(i);
    if (item->dot() == item->end())
    {
      item_completion(items, item, i);
    }
  }
}

#ifdef NEW_GRAMMAR
void
Parser::insert_transitions(ItemSetCore* core,
  const grammar::Symbol& symbol, size_t index)
{
  SetSymbolRules tuple{core, symbol, {}};
  auto result = insert_transition(tuple);
  std::get<1>(result)->transitions.push_back(index);
}
#endif

#ifndef NEW_GRAMMAR
void
Parser::insert_transitions(ItemSetCore* core, const Entry& symbol, size_t index)
{
  const auto& scanner = get<Scanner>(symbol);

  if (scanner.right == -1)
  {
    SetSymbolRules tuple{core, scanner.left, {}};
    auto [inserted, iter] = insert_transition(tuple);
    iter->transitions.push_back(index);

    (void)inserted;
  }
  else
  {
    auto current = scanner.left;
    while (current <= scanner.right)
    {
      SetSymbolRules tuple{core, current, {}};
      auto [inserted, iter] = insert_transition(tuple);
      iter->transitions.push_back(index);
      ++current;

      (void)inserted;
    }
  }
}
#endif

// If this item has a symbol after the dot, add an index for
// (current item set, item->symbol) -> item
void
Parser::item_transition(ItemSet* items, const PItem* item, size_t index)
{
  auto& rule = item->rule();
  auto core = items->core();

  if (item->dot() != rule.end())
  {
    auto& symbol = *item->dot();

    if (is_terminal(symbol))
    {
      insert_transitions(items->core(), symbol, index);
    }
    else
    {
      SetSymbolRules tuple{core, get_symbol(symbol), {}};
      auto [inserted, iter] = insert_transition(tuple);
      if (inserted)
      {
        // prediction
        // insert initial items for this symbol
#ifdef NEW_GRAMMAR
        for (auto& prediction: m_grammar_new.rules(get_terminal(symbol)))
#else
        for (auto& prediction: m_grammar.get(get_terminal(symbol)))
#endif
        {
          add_initial_item(core, get_item(&prediction, 0));
        }
      }
      iter->transitions.push_back(index);
    }

    // if this symbol can derive empty then add the next item too
    if (nullable(symbol) && item->dot() != rule.end())
    {
      // nullable completion
      add_initial_item(core, get_item(&rule, item->dot() - rule.begin() + 1));
    }
  }
}

void
Parser::item_completion(ItemSet*, const PItem*, size_t)
{
  // Completion in the current set:
  // Find the rules that predicted the lhs of this rule in the current set
  //SetSymbolRules tuple{items->core(), item->rule().nonterminal(), {}};
  //auto iter = m_set_symbols.find(tuple);

  //assert(iter != m_set_symbols.end());
};

auto
Parser::insert_transition(const SetSymbolRules& tuple)
-> std::tuple<bool, decltype(m_set_symbols.find(tuple))>
{
  auto iter = m_set_symbols.insert(tuple);
  bool inserted = false;

  if (iter.second)
  {
    //insert a new set
    iter.first->transitions.reserve(64);
    inserted = true;
  }

  return std::make_tuple(inserted, iter.first);
}

void
Parser::add_initial_item(ItemSetCore* core, const PItem* item)
{
  // add the item if it doesn't already exist
  for (size_t i = core->start_items(); i != core->all_items(); ++i)
  {
    if (item == core->item(i))
    {
      return;
    }
  }

  core->add_initial_item(item);
}

// Do scans and completions to start the current set
// find it in the hash table, then expand it if it's new
std::shared_ptr<ItemSet>
Parser::create_new_set(size_t position, const TerminalList& input)
{
  auto symbol = input[position];
  auto token = create_token(symbol);
  auto core = std::make_unique<ItemSetCore>();
  auto current_set = std::make_shared<ItemSet>(core.get());

  auto previous_set = m_itemSets[position];
  auto& previous_core = *previous_set->core();

  // look up the symbol index for the previous set
  auto scans = m_set_symbols.find(SetSymbolRules{
    &previous_core, token, {}});

  if (scans != m_set_symbols.end())
  {
    // do all the scans
    for (auto transition: scans->transitions)
    {
      auto item = previous_core.item(transition);
      auto next = get_item(&item->rule(), item->dot_index() + 1);

      if (position < input.size()-1 &&
          !next->in_lookahead(input[position+1]))
      {
        continue;
      }

      current_set->add_start_item(
        next,
        previous_set->actual_distance(transition) + 1);
    }

    // now do all the completed items
    for (size_t i = 0; i < core->start_items(); ++i)
    {
      auto item = core->item(i);
      auto& rule = item->rule();
      // TODO: change this to empty tail
      if (item->dot() == rule.end())
      {
        auto distance = current_set->distance(i);
        auto from = position - distance + 1;
        auto from_set = m_itemSets.at(from);
        auto from_core = from_set->core();

        // find the symbol for the lhs of this rule in set that predicted this
        // i.e., this is a completion: find the items it completes
        auto transitions = m_set_symbols.find(SetSymbolRules{
          from_core,
#ifdef NEW_GRAMMAR
          {item->rule().nonterminal(), false},
#else
          item->rule().nonterminal(),
#endif
          {}
        });

        if (transitions == m_set_symbols.end())
        {
          if (static_cast<size_t>(item->rule().nonterminal()) != m_grammar_new.start())
          {
#if 0
            std::cerr << "At position " << position << ", Completing item: ";
            item->print(std::cerr, m_names);
            std::cerr << ": " << current_set->distance(i) << std::endl;
            std::cerr << "Can't find " << m_names[item->rule().nonterminal()]
              << " in set " << from << std::endl;
            for (size_t print_i = 0; print_i <= position; ++print_i)
            {
              std::cerr << "-- Set " << print_i << " -- " << std::endl;
              print_set(print_i, m_names);
            }
#endif
            exit(1);
          }

          continue;
        }

        for (auto transition: transitions->transitions)
        {
          auto* item = from_core->item(transition);
          auto* next = get_item(&item->rule(), item->dot() - item->rule().begin() + 1);

          if (position < input.size()-1 &&
              !next->in_lookahead(input[position+1]))
          {
            continue;
          }
          current_set->add_start_item(next, 
            from_set->actual_distance(transition) + distance);
        }
      }
    }
  }
  else
  {
    std::cerr << "Couldn't find token " << symbol << " in set " << position << std::endl;
  }

  core.release();
  return current_set;
}

void
Parser::print_set(size_t i)
{
  auto set = m_itemSets.at(i);
  auto& names = m_grammar_new.names();
  set->print({names.begin(), names.end()});
}

void
ItemSet::print(const std::unordered_map<size_t, std::string>& names) const
{
  size_t i = 0;

  for (; i != m_core->start_items(); ++i)
  {
    m_core->item(i)->print(std::cout, names);
    std::cout << ": " << m_distances[i] << std::endl;
  }

  std::cout << "--------" << std::endl;

  for (; i != m_core->all_items(); ++i)
  {
    m_core->item(i)->print(std::cout, names);
    std::cout << std::endl;
  }
}

}
