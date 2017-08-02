#include "cxxopts.hpp"
#include "earley.hpp"
#include "grammar.hpp"
#include "numbers.hpp"

int main(int argc, char** argv)
{
  using namespace earley;

  cxxopts::Options options("earley", "an earley parser");
  options.add_options()
    ("d,debug", "turn on debugging")
    ("expression", "parse a simple expression", cxxopts::value<std::string>())
    ("h,help", "show help")
    ("t,timing", "print timing")
  ;

  options.parse(argc, argv);

  if (options.count("help"))
  {
    std::cout << options.help();
    exit(1);
  }

  bool debug = options.count("debug");
  bool timing = options.count("timing");

  if (options.count("expression"))
  {
    parse_expression(options["expression"].as<std::string>(), debug, timing);
  }

  Grammar ebnf = {
    {"Grammar", {
      {{"Nonterminal"}, {"create_list", {0}}},
      {{"Grammar", "Space", "Nonterminal"}, {"append_list", {0, 1}}},
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
      {{'[', "Ranges", ']'}, {"pass", {1}}},
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

  if (argc < 2)
  {
    std::cerr << "Give me an ebnf to parse" << std::endl;
    exit(0);
  }

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
    process_input(debug, ebnf_ids["Grammar"], argv[1], ebnf_rules, ebnf_ids);
  (void)ebnf_items;
  (void)ebnf_pointers;

  if (!ebnf_parsed)
  {
    std::cerr << "Unable to parse ebnf" << std::endl;
    exit(1);
  }
  else
  {
    std::cout << ebnf_time << std::endl;

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
        ebnf_pointers, ebnf_ids["Grammar"], argv[1], actions, ebnf_items, ebnf_ids);

    print_grammar(value);
  }

  return 0;
}
