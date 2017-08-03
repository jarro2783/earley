#include <typeinfo>

#include "grammar.hpp"

namespace earley
{

namespace ast
{

class InvalidGrammar
{
};

namespace
{

template <typename T>
void
check(GrammarNode node)
{
  if (!holds<T>(node))
  {
    std::cerr << "Holds " << node.index() << std::endl;
    throw InvalidGrammar();
  }
}

template <typename T, typename U>
T
checked_cast(U* u)
{
  T t = dynamic_cast<T>(u);
  if (t == nullptr)
  {
    std::cerr << "Type is: " << typeid(*u).name() << std::endl;
    throw InvalidGrammar();
  }

  return t;
}

void
print_rule(GrammarPtr ptr)
{
  auto rule = checked_cast<const Rule*>(ptr.get());
  auto& productions = rule->productions();

  for (auto& p: productions)
  {
    if (holds<std::string>(p))
    {
      std::cout << " " << get<std::string>(p);
    }
    // TODO: implement Scanner better and print it here
    else if (holds<Scanner>(p))
    {
      std::cout << "'scanfn'";
    }
  }
}

void
print_nonterminal(GrammarNode nt)
{
  check<GrammarPtr>(nt);
  auto ptr = get<GrammarPtr>(nt);
  auto node = checked_cast<const earley::ast::GrammarNonterminal*>(ptr.get());

  std::cout << node->name() << " ->";

  auto& rules = node->rules();
  auto iter = rules.begin();

  if (iter != rules.end())
  {
    print_rule(*iter);
    ++iter;
  }

  while (iter != rules.end())
  {
    std::cout << std::endl << "  |";
    print_rule(*iter);
    ++iter;
  }
  std::cout << std::endl;
}

}

void
print_grammar(GrammarNode grammar)
{
  check<GrammarPtr>(grammar);

  auto ptr = get<GrammarPtr>(grammar);
  auto list = checked_cast<const GrammarList*>(ptr.get());

  for (auto& nt : list->list())
  {
    print_nonterminal(nt);
  }
}

std::ostream&
operator<<(std::ostream& os, const GrammarRange& range)
{
  os << "[" << range.m_begin << "-" << range.m_end << "]";
  return os;
}

}

void
parse_ebnf(const std::string& input, bool debug, bool timing)
{
  Grammar ebnf = {
    {"Grammar", {
      {{"Nonterminal"}, {"create_list", {0}}},
      {{"Grammar", "HardSpace", "Nonterminal"}, {"append_list", {0, 2}}},
    }},
    {"Space", {
      {{Epsilon()}},
      {{"SpaceRest"}},
    }},
    {"HardSpace", {
      {{"SpaceRest"}},
    }},
    {"SpaceRest", {
      {{"SpaceChar"}},
      {{"SpaceRest", "SpaceChar"}},
    }},
    {"SpaceChar", {
      {{' '}},
      {{'\n'}},
      {{'\t'}},
    }},
    {"Nonterminal", {
      {{"Name", "Space", '-', '>', "Rules"}, {"create_nonterminal", {0, 4}}}
    }},
    {"Rules", {
      {{"Rule"}, {"create_list", {0}}},
      {{"Rules", "Space", '|', "Rule"}, {"append_list", {0, 3}}},
    }},
    {"Rule", {
      //{{Epsilon()}, {"rule", {}}},
      {{"Productions"}, {"rule", {0}}},
      {{"Productions", "Action"}, {"rule", {0, 1}}},
    }},
    {"Productions", {
      {{"Production"}, {"create_list", {0}}},
      {{"Productions", "HardSpace", "Production"}, {"append_list", {0, 2}}},
    }},
    {"Production", {
      {{"Name"}, {"pass", {0}}},
      {{"Space", "Literal"}, {"pass", {1}}},
    }},
    {"Action", {
      {{"Space", '#', "Space", "Name"}},
    }},
    {"Literal", {
      {{'\'', "Char", '\''}, {"pass", {1}}},
      {{'[', "Range", ']'}, {"pass", {1}}},
      {{'"', "Chars", '"'}, {"pass", {1}}},
    }},
    {"Name", {
      {{"Space", "NameStart"}, {"pass", {1}}},
      {{"Space", "NameStart", "NameRest"}, {"append_string", {1, 2}}},
    }},
    {"NameStart", {
      {{scan_range('a', 'z')}, {"create_string", {0}}},
      {{scan_range('A', 'Z')}, {"create_string", {0}}},
    }},
    {"NameRest", {
      {{"NameStart"}, {"pass", {0}}},
      {{"NameRest", "NameStart"}, {"append_string", {0, 1}}},
      {{"NameRest", scan_range('0', '9')}, {"append_string", {0, 1}}},
    }},
    {"Ranges", {
      {{"Range"}, {"create_list", {0}}},
      {{"Ranges", "Range"}, {"append_list", {0, 1}}},
    }},
    {"Range", {
      {{scan_range('a', 'z'), '-', scan_range('a', 'z')}, {"create_range", {0, 2}}},
      {{scan_range('A', 'Z'), '-', scan_range('A', 'Z')}, {"create_range", {0, 2}}},
      {{scan_range('0', '9'), '-', scan_range('0', '9')}, {"create_range", {0, 2}}},
    }},
    {"Chars", {
      {{"Char"}, {"create_list", {0}}},
      {{"Chars", "Char"}, {"append_list", {0}}},
    }},
    {"Char", {
      {{scan_range(' ', '~')}, {"pass", {0}}},
    }},
  };

  auto [ebnf_rules, ebnf_ids] = generate_rules(ebnf);

  if (debug)
  {
    std::cout << "Generated grammar rules:" << std::endl;
    for (auto& id : ebnf_ids)
    {
      std::cout << id.first << " = " << id.second << std::endl;
    }
  }

  auto [ebnf_parsed, ebnf_time, ebnf_items, ebnf_pointers] =
    process_input(debug, ebnf_ids["Grammar"], input, ebnf_rules, ebnf_ids);
  (void)ebnf_items;
  (void)ebnf_pointers;

  if (!ebnf_parsed)
  {
    std::cerr << "Unable to parse ebnf" << std::endl;
    exit(1);
  }
  else
  {
    if (timing)
    {
      std::cout << ebnf_time << std::endl;
    }

    std::unordered_map<std::string, ast::GrammarNode(*)(
      std::vector<ast::GrammarNode>&
    )> actions;

    using namespace earley::ast;

    add_action("pass", actions, &handle_pass);
    add_action("create_list", actions, &ast::action_create_list);
    add_action("append_list", actions, &ast::action_append_list);
    add_action("create_string", actions, &ast::action_create_string);
    add_action("append_string", actions, &ast::action_append_string);
    add_action("rule", actions, &ast::action_rule);
    add_action("create_range", actions, &ast::action_create_range);
    add_action("create_nonterminal", actions, &ast::action_create_nonterminal);

    auto value = earley::run_actions(
        ebnf_pointers, ebnf_ids["Grammar"], input, actions, ebnf_items, ebnf_ids);

    print_grammar(value);
  }
}

}
