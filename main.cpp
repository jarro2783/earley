#include "cxxopts.hpp"
#include "earley.hpp"

int main(int argc, char** argv)
{
  using namespace earley;

  cxxopts::Options options("earley", "an earley parser");
  options.add_options()
    ("d,debug", "turn on debugging")
    ("t,timing", "print timing")
  ;

  options.parse(argc, argv);

  bool debug = options.count("debug");
  bool timing = options.count("timing");

  Rule parens(0, {scan_char('('), 0ul, scan_char(')')});
  Rule empty(0, {earley::Epsilon()});

  std::vector<RuleList> rules{
    {parens, empty,},
  };

  process_input(debug, 0, "()", rules);
  process_input(debug, 0, "(())", rules);
  process_input(debug, 0, "((", rules);

  // number -> space | number [0-9]
  // sum -> product | sum + product
  // product -> number | product * number
  // space -> | space [ \t\n]
  // input -> sum space
  std::unordered_map<size_t, RuleList> numbers{
    {
      0,
      {
        {0, {3ul}},
        {0, {0ul, '0'}},
        {0, {0ul, '1'}},
        {0, {0ul, '2'}},
        {0, {0ul, '3'}},
        {0, {0ul, '4'}},
        {0, {0ul, '5'}},
        {0, {0ul, '6'}},
        {0, {0ul, '7'}},
        {0, {0ul, '8'}},
        {0, {0ul, '9'}},
      },
    },
    {
      1,
      {
        {1, {2ul}},
        {1, {1ul, 3ul, '+', 2ul}},
      },
    },
    {
      2,
      {
        {2, {0ul}},
        {2, {2ul, 3ul, '*', 0ul}},
      },
    },
    {
      3,
      {
        {3, {Epsilon()}},
        {3, {3ul, ' '}},
        {3, {3ul, '\t'}},
        {3, {3ul, '\n'}},
      },
    },
    {
      4,
      {
        {4, {1ul, 3ul}},
      },
    },
  };

  Grammar grammar = {
    {"Number", {
      {"Space", scan_range('0', '9')},
      {"Number", scan_range('0', '9')},
    }},
    {"Space", {
      {Epsilon()},
      {"Space", scan_char(' ')},
      {"Space", scan_char('\t')},
      {"Space", scan_char('\n')},
    }},
    {"Sum", {
      {"Product"},
      {"Sum", "Space", scan_char('+'), "Product"},
      {"Sum", "Space", scan_char('-'), "Product"},
    }},
    {"Product", {
      {"Number"},
      {"Product", "Space", scan_char('*'), "Number"},
      {"Product", "Space", scan_char('/'), "Number"},
    }},
    {"Input", {{"Sum", "Space"}}},
  };

  if (argc < 2)
  {
    std::cerr << "Give me an expression to parse" << std::endl;
    exit(1);
  }

  auto [grammar_rules, ids] = generate_rules(grammar);

  if (debug)
  {
    std::cout << "Generated grammar rules:" << std::endl;
    for (auto& id : ids)
    {
      std::cout << id.first << " = " << id.second << std::endl;
    }

    size_t rule_id = 0;
    for (auto& rule : grammar_rules)
    {
      std::cout << rule_id << ":" << std::endl;
      for (auto& item : rule)
      {
        std::cout << item << std::endl;
      }
      ++rule_id;
    }
  }

  auto [success, elapsed] =
    process_input(debug, ids["Input"], argv[1], grammar_rules);

  if (!success)
  {
    std::cerr << "Unable to parse" << std::endl;
  } else if (timing)
  {
    std::cout << elapsed << std::endl;
  }

  Grammar ebnf = {
    {"Grammar", {
      {Epsilon()},
      {"Grammar", "Space", "Nonterminal"}
    }},
    {"Space", {
      {Epsilon()},
      {' '},
    }},
    {"Nonterminal", {
      {"Name", '-', '>', "Rules"}
    }},
    {"Rules", {
      {"Rules", '|', "Rule"},
      {"Rule"},
    }},
    {"Rule", {
      {Epsilon()},
      {"Rule", "Production"},
    }},
    {"Production", {
      {"Name"},
      {"Literal"}
    }},
    {"Literal", {
      {'\'', "Char", '\''},
      {'[', "Ranges", ']'},
      {'"', "Chars", '"'},
    }},
    {"Name", {
      {"NameStart", "NameRest"},
    }},
    {"NameStart", {
      {scan_range('a', 'z')},
      {scan_range('A', 'Z')},
    }},
    {"NameRest", {
      {Epsilon()},
      {"NameRest", "NameStart"},
      {"NameRest", scan_range('0', '9')},
    }},
    {"Ranges", {
      {"Range"},
      {"Ranges", "Range"},
    }},
    {"Range", {
      {scan_range('a', 'z'), '-', scan_range('a', 'z')},
      {scan_range('A', 'Z'), '-', scan_range('A', 'Z')},
      {scan_range('0', '9'), '-', scan_range('0', '9')},
    }},
    {"Chars", {
      {"Char"},
      {"Chars", "Char"},
    }},
    {"Char", {
      {scan_range('0', 'z')},
      {'('},
      {')'},
    }},
  };

  if (argc < 3)
  {
    std::cerr << "Give me an ebnf to parse" << std::endl;
    exit(1);
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

  auto [ebnf_parsed, ebnf_time] =
    process_input(debug, ebnf_ids["Grammar"], argv[2], ebnf_rules);

  return 0;
}
