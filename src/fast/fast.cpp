#include <cassert>

#include "earley/fast.hpp"
#include "earley/grammar_util.hpp"

namespace earley::fast
{

namespace
{
  template <typename T>
  void
  insert_unique(std::vector<T>& c, const T& v)
  {
    auto iter = std::find(c.begin(), c.end(), v);

    if (iter == c.end())
    {
      c.push_back(v);
    }
  }

  template <typename T>
  auto
  insert_unique(HashSet<T>& c, const T& v)
  {
    return c.insert(v);
  }

  bool
  compare_lookahead_sets(std::vector<ItemSet*>& item_sets,
    ItemSet* a, int place, int position)
  {
    auto& da = a->distances();

    for (size_t i = 0; i != a->core()->start_items(); ++i)
    {
      if (item_sets[place - da[i]] != item_sets[position + 1 - da[i]])
      {
        return false;
      }
    }

    return true;
  }

  bool
  in_start_items(const Item* item, const ItemSet* set)
  {
    auto core = set->core();
    auto items = core->start_item_list();
    return std::find(items.begin(), items.end(), item) != items.end();
  }
}

void
Parser::unique_insert_start_item(
  ItemSet* set,
  const Item* item,
  int distance,
  int position
)
{
  // We are only ever looking at the latest set, so a neat optimisation
  // here is to keep a single structure called `membership`, which is a map
  // of item -> array of position.
  // When we insert an item with a distance `distance`, when parsing token
  // `position`, then we set
  //   membership[item][distance] = position
  // Then if `membership[item][distance] == position` we know that we have
  // already added that item.
  // Otherwise `membership[item][distance]` will always be less than position.

  auto& dots = m_item_membership[item->index()];
  if (dots.size() <= static_cast<size_t>(distance))
  {
    // micro optimisation: we don't know how big this distance could ever be
    // so double the distance to reduce allocations sometimes
    dots.resize((distance+1)*2);
  }
  else if (dots[distance] == static_cast<size_t>(position))
  {
    return;
  }

  dots[distance] = position;
  set->add_start_item(item, distance);
}

grammar::Symbol
create_token(int c)
{
  return grammar::Symbol{c, true};
}

void
ItemSet::add_start_item(const PItem* item, size_t distance)
{
  hash_combine(m_hash, std::hash<const PItem*>()(item));
  //hash_combine(m_hash, distance);

  m_core->add_start_item(item);
  m_distances.append(distance);
}

Parser::Parser(const grammar::Grammar& grammar_new, const TerminalList& tokens)
: m_grammar_new(grammar_new)
, m_tokens(tokens)
, m_item_set_hash(tokens.size() < 20000 ? 20000 : tokens.size() / 5)
, m_set_symbols(tokens.size() < 20000 ? 20000 : tokens.size())
, m_set_term_lookahead(tokens.size() < 30000 ? 30000 : tokens.size())
, m_distance_hash(tokens.size() < 20000 ? 20000 : tokens.size() / 5)
, m_all_items(m_grammar_new.all_rules(),
    m_grammar_new.first_sets(),
    m_grammar_new.follow_sets(),
    m_grammar_new.nullable_set())
{
  m_item_membership.resize(m_all_items.items());

  m_setOwner.reserve(tokens.size());
  m_coreOwner.reserve(tokens.size());

  assert(m_setOwner.capacity() == tokens.size());
  assert(m_coreOwner.capacity() == tokens.size());

  m_itemSets.reserve(tokens.size()+1);
  create_start_set();
}

void
Parser::parse_input()
{
  size_t position = 0;
  while (position < m_tokens.size())
  {
    parse(position);
    ++position;
  }

  std::cout << "reused " << m_reuse << std::endl;
  std::cout << m_tokens.size() << " tokens" << std::endl;
}

void
Parser::parse(size_t position)
{
  auto token = m_tokens[position];
  auto lookahead = position < m_tokens.size()-1
    ? m_tokens[position+1]
    : -1;

  auto lookahead_hash = m_set_term_lookahead.insert(
    SetTermLookahead(
      m_itemSets[position],
      token,
      lookahead));

  if (!lookahead_hash.second)
  {
    if (lookahead_hash.first->goto_count > 0)
    {
      int which = 0;
      while (which < lookahead_hash.first->goto_count)
      {
        auto place = lookahead_hash.first->place[which];
        if (compare_lookahead_sets(m_itemSets, lookahead_hash.first->goto_sets[which],
          place, position))
        {
          ++m_reuse;
          m_itemSets.push_back(m_itemSets[place]);
          return;
        }
        ++which;
      }
    }
    ++m_lookahead_collisions;
  }

  auto set = create_new_set(position, m_tokens);

  auto core_hash = m_set_core_hash.insert(set->core());

  auto distance_hash = m_distance_hash.insert(set->distances());

  if (!distance_hash.second)
  {
    set->distances().reset();
    set->set_distance(*distance_hash.first);
  }
  else
  {
    set->distances().finalise();
  }

  if (!core_hash.second)
  {
    set->set_core(*core_hash.first);
    m_core_reset = true;
  }

  set->finalise();

  // if this is a new set, then expand it
  //auto copy = *set;
  auto result = m_item_set_hash.insert(set);

  if (!result.second)
  {
    reset_set();
  }

  if (core_hash.second)
  {
    expand_set(&result.first->get());
  }

  // keeping the most recent set here seems to increase reuse a bit
  auto& goto_count = lookahead_hash.first->goto_count;
  lookahead_hash.first->goto_sets[goto_count] = &result.first->get();
  lookahead_hash.first->place[goto_count] = position+1;
  goto_count = (goto_count+1) % MAX_LOOKAHEAD_SETS;

  if (core_hash.second)
  {
    (*core_hash.first)->finalise();
  }
  m_itemSets.push_back(&result.first->get());
}

void
Parser::create_start_set()
{
  auto& core = m_coreOwner.emplace_back();
  auto items = &m_setOwner.emplace_back(&core);

  for (auto& rule: m_grammar_new.rules(m_grammar_new.start()))
  {
    auto item = get_item(&rule, 0);
    items->add_start_item(item, 0);
  }

  expand_set(items);

  m_itemSets.push_back(items);
  m_item_set_hash.insert(items);

  core.finalise();
  items->distances().finalise();
  items->finalise();
}

void
__attribute__((noinline))
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

  //for (size_t i = 0; i < core->all_items(); ++i)
  //{
  //  auto item = core->item(i);
  //  if (item->dot() == item->end())
  //  {
  //    item_completion(items, item, i);
  //  }
  //}
}

void
Parser::insert_transitions(ItemSetCore* core,
  const grammar::Symbol& symbol, size_t index)
{
  SetSymbolRules tuple(core, symbol);
  auto result = insert_transition(tuple);
  result.first->second.push_back(index);
}

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
      SetSymbolRules tuple(core, get_symbol(symbol));
      auto [iter, inserted] = insert_transition(tuple);
      if (inserted)
      {
        // prediction
        // insert initial items for this symbol
        for (auto& prediction: m_grammar_new.rules(get_terminal(symbol)))
        {
          add_initial_item(core, get_item(&prediction, 0));
        }
      }
      iter->second.push_back(index);
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
-> decltype(m_set_symbols.emplace(tuple))
{
  //std::cout << "Inserting (" << tuple.set << ", (" << tuple.symbol.index << ", " << tuple.symbol.terminal << "))" << std::endl;
  return m_set_symbols.emplace(tuple);
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
ItemSet*
Parser::create_new_set(size_t position, const TerminalList& input)
{
  auto symbol = input[position];
  auto token = create_token(symbol);
  auto& core = next_core();
  auto current_set = &next_set(&core);

  auto previous_set = m_itemSets[position];
  auto& previous_core = *previous_set->core();

  // look up the symbol index for the previous set
  auto scans = m_set_symbols.find(SetSymbolRules(
    &previous_core, token));

  if (scans != m_set_symbols.end())
  {
    // do all the scans
    for (auto transition: scans->second)
    {
      auto item = previous_core.item(transition);
      auto next = get_item(&item->rule(), item->dot_index() + 1);

      if (position < input.size()-1 &&
          !next->in_lookahead(input[position+1]))
      {
        continue;
      }

      unique_insert_start_item(current_set, next,
        previous_set->actual_distance(transition)+1, position);

      //auto pointers = m_item_tree.insert({next,
      //  current_set,
      //  previous_set->actual_distance(transition) + 1,
      //});
      //insert_unique(pointers.first->predecessor, item);
    }

    // now do all the completed items
    for (size_t i = 0; i < core.start_items(); ++i)
    {
      auto item = core.item(i);
      //auto& rule = item->rule();

      if (item->empty_rhs())
      {
        auto distance = current_set->distance(i);
        auto from = position - distance + 1;
        auto from_set = m_itemSets.at(from);
        auto from_core = from_set->core();

        // find the symbol for the lhs of this rule in set that predicted this
        // i.e., this is a completion: find the items it completes
        auto transitions = m_set_symbols.find(SetSymbolRules(
          from_core,
          {item->rule().nonterminal(), false}
        ));

        if (transitions == m_set_symbols.end())
        {
          if (item->rule().nonterminal() != m_grammar_new.start())
          {
            //TODO: clean this up
            auto& names = m_grammar_new.names();
            std::cerr << "At position " << position << ", Completing item: ";
            item->print(std::cerr, {names.begin(), names.end()});
            std::cerr << ": " << current_set->distance(i) << std::endl;
            auto iter = names.find(item->rule().nonterminal());
            std::cerr << "Can't find " << (iter != names.end() ? iter->second : "")
              << " in set " << from << std::endl;
            for (size_t print_i = 0; print_i <= position; ++print_i)
            {
              std::cerr << "-- Set " << print_i << " -- " << std::endl;
              print_set(print_i);
            }
            exit(1);
          }

          continue;
        }

        for (auto transition: transitions->second)
        {
          auto* titem = from_core->item(transition);
          auto* next = get_item(&titem->rule(),
            titem->dot() - titem->rule().begin() + 1);

          if (position < input.size()-1 &&
              !next->in_lookahead(input[position+1]))
          {
            continue;
          }

          auto transition_distance = from_set->actual_distance(transition) + distance;
          unique_insert_start_item(current_set, next,
            transition_distance, position);

          // If we are actually at the end, then add a reduction
          // We can do this later, once for each unique set
          // If we go through each unique set when we are finished, then we
          // can take every completed item t, find the item that predicted it q,
          // create the next item p, then make a reduction pointer p to t, and 
          // a predecessor pointer p to q.
#if 0
          if (item->position() == item->end())
          {
            auto pointers = m_item_tree.insert({next, current_set,
              from_set->actual_distance(transition) + distance});
            insert_unique(pointers.first->reduction, item);
            insert_unique(pointers.first->predecessor, titem);
          }
#endif
        }
      }
    }
  }
  else
  {
    std::cerr << "Couldn't find token " << symbol << " in set " << position << std::endl;
    parse_error(position);
    throw "Parse error";
  }

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

  std::cout << "  core: " << m_core << std::endl;
  std::cout << "  Set core = " << m_core->number() << std::endl;
  std::cout << "  Start items: " << m_core->start_items() << std::endl;


  for (; i != m_core->start_items(); ++i)
  {
    m_core->item(i)->print(std::cout, names);
    std::cout << ": " << m_distances[i] << std::endl;
  }

  std::cout << "--------" << std::endl;

  for (; i != m_core->all_items(); ++i)
  {
    m_core->item(i)->print(std::cout, names);

    if (i < m_core->all_distances())
    {
      std::cout << ": " << m_core->parent_distance(i);
    }

    std::cout << std::endl;
  }
}

void
Parser::parse_error(size_t i)
{
  //failed here
  std::cout << "Parse error at " << i << ", expecting: ";

  auto& names = m_grammar_new.names();
  std::unordered_map<size_t, std::string> item_names(names.begin(), names.end());

  //look for all the scans and print out what we were expecting
  auto set = m_itemSets[i];
  auto core = set->core();

  for (auto item: core->items())
  {
    auto symbol = item->position();
    if (symbol != item->end() && symbol->terminal)
    {
      auto token = symbol->index;

      if (token <= 127 && token >= ' ')
      {
        std::cout << "'" << static_cast<char>(token) << "'";
      }
      else
      {
        std::cout << token;
      }
      std::cout << ", ";
    }

    item->print(std::cout, item_names);
    std::cout << std::endl;
  }

  std::cout << std::endl;
}

void
Parser::reset_set()
{
  //m_set_reset = true;
  m_setOwner.pop_back();
}

ItemSetCore&
Parser::next_core()
{
  if (m_core_reset)
  {
    m_coreOwner.back().reset();
    m_core_reset = false;
    return m_coreOwner.back();
  }

  auto& core = m_coreOwner.emplace_back();
  core.number(m_coreOwner.size() - 1);
  return core;
}

ItemSet&
Parser::next_set(ItemSetCore* core)
{
  if (m_set_reset)
  {
    m_setOwner.back().reset(core);
    m_set_reset = false;
    return m_setOwner.back();
  }

  return m_setOwner.emplace_back(core);
}

void
Parser::print_stats() const
{
  std::cout << "Hash set cores: " << m_set_core_hash.size() << std::endl;
  std::cout << "Unique cores: " << m_coreOwner.size() << std::endl;
  std::cout << "Goto collisions: " << m_lookahead_collisions << std::endl;
  std::cout << "Goto successes: " << m_reuse << std::endl;
  std::cout << "Unique sets: " << m_setOwner.size() << std::endl;
  std::cout << "Unique distances: " << m_distance_hash.size() << std::endl;
}

void
Parser::create_reductions()
{
  HashSet<ItemSet*> items_seen;
  size_t reductions = 0;
  size_t skipped_sets = 0;
  size_t skipped_items = 0;

  // This is almost the same algorithm as doing the completions of the initial
  // items.
  // We need to go from 1 to the end, because the first set won't have
  // completed anything.

  // The main difference is that we are counting the index in the item sets
  // not the token position, so there is a +1 that doesn't appear here.

  for (size_t position = 1; position < m_itemSets.size(); ++position)
  {
    auto item_set = m_itemSets[position];
    if (!items_seen.insert(item_set).second)
    {
      ++skipped_sets;
      continue;
    }

    auto core = item_set->core();
    auto start_items = core->start_items();

#ifdef DEBUG_REDUCTION
    std::cout << core->number() << ": " << start_items << " ";
#endif
    for (size_t i = 0; i != start_items; ++i)
    {
      auto item = core->item(i);
#ifdef DEBUG_REDUCTION
      std::cout << item << " ";
#endif

      if (item == nullptr)
      {
        std::cout << "Fail at " << position << ":" << i << std::endl;
        throw "fail";
      }

      if (item->empty_rhs())
      {
        auto distance = item_set->distance(i);
        auto from = position - distance; // no +1 because we're counting sets
        auto from_set = m_itemSets.at(from);
        auto from_core = from_set->core();

        // find the symbol for the lhs of this rule in set that predicted this
        // i.e., this is a completion: find the items it completes
        auto transitions = m_set_symbols.find(SetSymbolRules(
          from_core,
          {item->rule().nonterminal(), false}
        ));

        if (transitions == m_set_symbols.end())
        {
          // This should never happen, because we will only ever get here
          // if there is a successful parse.
          if (item->rule().nonterminal() != m_grammar_new.start())
          {
            auto names = m_grammar_new.names();
            std::unordered_map<size_t, std::string> item_names(
              names.begin(), names.end());

            for (auto& name: names)
            {
              std::cout << name.first << ", " << name.second << std::endl;
            }

            std::cerr << "Unexpected error finding transition " <<
              print_nt(item_names, item->rule().nonterminal())
              << " in set " << from
                      << " at position "
                      << position
                      << std::endl;
            throw "Error";
          }
          continue;
        }

        for (auto transition: transitions->second)
        {
          auto* titem = from_core->item(transition);
          auto* next = get_item(&titem->rule(),
            titem->dot() - titem->rule().begin() + 1);

          // In the other algorithm we check lookahead. Here we check whether
          // we already have this item in our set, because otherwise we don't
          // care about it.
          if (!in_start_items(next, item_set))
          {
            ++skipped_items;
            continue;
          }

          auto transition_distance = from_set->actual_distance(transition) + distance;

          // We only do the reduction if it is actually at the end
          if (item->position() == item->end())
          {
            auto pointers = m_item_tree.insert({next, item_set,
              transition_distance});
            insert_unique(pointers.first->reduction, item);
            insert_unique(pointers.first->predecessor, titem);
            ++reductions;
          }
        }
      }
#ifdef DEBUG_REDUCTION
      std::cout << std::endl;
#endif
    }
  }
  std::cout << "Added " << reductions << " reductions" << std::endl;
  std::cout << "Skipped " << skipped_sets << " sets" << std::endl;
  std::cout << "Skipped " << skipped_items << " items" << std::endl;
}

Stack<const Item*> ItemSetCore::item_stack;
Stack<int> ItemSetCore::parent_stack;
Stack<int> ItemSet::distance_stack;

}
