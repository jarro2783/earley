#include <chrono>
#include <typeinfo>

#include "grammar.hpp"
#include "earley/fast.hpp"

#include "earley/fast/grammar.hpp"

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

std::string
escape(char c)
{
  switch (c)
  {
    case '\t':
    return "\\t";

    case '\n':
    return "\\n";

    default:
    return std::string(1, c);
  }
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
    else if (holds<Scanner>(p))
    {
      std::cout << ' ';
      get<Scanner>(p).print(std::cout);
    }
    else if (holds<char>(p))
    {
      std::cout << " '" << escape(get<char>(p)) << "'";
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

std::vector<RuleWithAction>
build_rules(const std::vector<GrammarPtr>& rules)
{
  std::vector<RuleWithAction> built;

  for (auto& ptr: rules)
  {
    auto rule = checked_cast<Rule*>(ptr.get());
    built.push_back(rule->productions());
  }

  return built;
}

std::string
add_nonterminal(earley::Grammar& grammar, GrammarNode tree)
{
  check<GrammarPtr>(tree);

  auto ptr = get<GrammarPtr>(tree);
  auto nt = checked_cast<GrammarNonterminal*>(ptr.get());

  grammar.insert({nt->name(), build_rules(nt->rules())});

  return nt->name();
}

std::tuple<earley::Grammar, std::string>
compile_grammar(GrammarNode tree)
{
  earley::Grammar grammar;

  check<GrammarPtr>(tree);

  auto ptr = get<GrammarPtr>(tree);
  auto list = checked_cast<const GrammarList*>(ptr.get());

  std::string start;

  for (auto& nt: list->list())
  {
    auto name = add_nonterminal(grammar, nt);
    if (start.size() == 0)
    {
      start = name;
    }
  }

  return {grammar, start};
}

}

std::ostream&
operator<<(std::ostream& os, const GrammarRange& range)
{
  os << "[" << range.m_begin << "-" << range.m_end << "]";
  return os;
}

void
parse(const earley::Grammar& grammar, const std::string& start,
  const std::string& text, bool debug, bool timing)
{
  if (debug)
  {
    std::cout << "Start symbol is " << start << std::endl;
  }

  auto [rules, ids] = generate_rules(grammar);
  auto [parsed, time, items, pointers] =
    process_input(debug, ids[start], text, rules, ids);

  (void)items;
  (void)pointers;

  if (parsed)
  {
    std::cout << "Parsed successfully";
  }
  else
  {
    std::cout << "Failed to parse";
  }
  std::cout << std::endl;
  if (timing)
  {
    std::cout << "Slow parser took " << time << " microseconds" << std::endl;
  }
}

}

void
parse_ebnf(const std::string& input, bool debug, bool timing, bool slow,
  const std::string& text)
{
  Grammar ebnf = {
    {"Grammar", {
      {{"Nonterminal"}, {"create_list", {0}}},
      {{"Grammar", "HardSpace", "Nonterminal"}, {"append_list", {0, 2}}},
    }},
    {"Space", {
      {{}},
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
      {{"Name", "Space", "RuleSeparator", "Rules", "OptSemi"},
        {"create_nonterminal", {0, 3}}}
    }},
    {"RuleSeparator", {
      {{'-', '>'}},
      {{':'}},
    }},
    {"OptSemi", {
      {{}},
      {{"Space", ';'}},
    }},
    {"Rules", {
      {{"Rule"}, {"create_list", {0}}},
      {{"Rules", "Space", '|', "Rule"}, {"append_list", {0, 3}}},
    }},
    {"Rule", {
      {{}, {"rule", {}}},
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
      {{"Space", '#', "Name", "HardSpace", "Numbers"}},
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
      {{scan_range('a', 'z')}, {"pass", {0}}},
      {{scan_range('0', '9')}, {"pass", {0}}},
      {{'\\', 't'}, {"escape", {1}}},
      {{'\\', 'n'}, {"escape", {1}}},
      {{' '}, {"pass", {0}}},
      {{'+'}, {"pass", {0}}},
      {{'-'}, {"pass", {0}}},
      {{'/'}, {"pass", {0}}},
      {{'*'}, {"pass", {0}}},
      {{'('}, {"pass", {0}}},
      {{')'}, {"pass", {0}}},
    }},
    {"Numbers", {
      {{"Space", "Number"}},
      {{"Numbers", "HardSpace", "Number"}},
    }},
    {"Number", {
      {{"Digit"}},
      {{"Number", "Digit"}},
    }},
    {"Digit", {
      {{scan_range('0', '9')}},
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
      std::cout << "ebnf: " << ebnf_time << std::endl;
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
    auto [built, start] = compile_grammar(value);

    if (text.size())
    {
      if (debug)
      {
        std::cout << "Parsing:" << std::endl;
        std::cout << text << std::endl;
      }

      if (slow)
      {
        parse(built, start, text, debug, timing);
      }

      //test the fast parser
      auto [rules, ids] = generate_rules(built);
      ParseGrammar parse_grammar(ids[start], rules);
      earley::fast::grammar::Grammar grammar_new(start, built);

      std::chrono::time_point<std::chrono::system_clock> start_time, end;
      start_time = std::chrono::system_clock::now();

      fast::Parser parser(grammar_new);

      if (debug)
      {
        std::cout << "-- Set 0 --" << std::endl;
        parser.print_set(0);
      }

      for (size_t i = 0; i < text.size(); ++i)
      {
        parser.parse(fast::TerminalList(text.begin(), text.end()), i);

        if (debug)
        {
          std::cout << "-- Set " << i+1 << " --" << std::endl;
          parser.print_set(i+1);
        }
      }

      end = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end-start_time;

      if (timing)
      {
        std::cout << "Fast parser took "
          << std::chrono::duration_cast<std::chrono::microseconds>(
            elapsed_seconds).count()
          << " microseconds" << std::endl;
      }
    }
  }
}

}
