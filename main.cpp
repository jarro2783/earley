#include "cxxopts.hpp"
#include "earley.hpp"

typedef earley::ActionResult<int> NumberResult;

template <typename T>
void
add_action(
  const std::string& name,
  std::unordered_map<std::string, T (*)(const std::vector<T>&)>& actions,
  T (*fun)(const std::vector<T>&)
)
{
  actions[name] = fun;
}

typedef std::vector<NumberResult> NumbersParts;

NumberResult
handle_digit(const NumbersParts& parts)
{
  if (parts.size() == 1)
  {
    auto& ch = parts[0];
    if (std::holds_alternative<char>(ch))
    {
      return std::get<char>(ch) - '0';
    }
    else
    {
      std::cerr << "Invalid type in handle_number[1]" << std::endl;
      return 0;
    }
  }

  return 0;
}

NumberResult
handle_number(const NumbersParts& parts)
{
  int value = 0;
  if (parts.size() == 2)
  {
    auto& integer = parts[0];
    auto& character = parts[1];

    if (std::holds_alternative<int>(integer) &&
        std::holds_alternative<char>(character))
    {
      value = std::get<int>(integer) * 10 + (std::get<char>(character) - '0');
    }
    else
    {
      std::cerr << "Invalid type in handle_number[2]" << std::endl;
    }
  }

  //std::cout << "Parsed: " << value << std::endl;
  return value;
}

NumberResult
handle_pass(const NumbersParts& parts)
{
  return parts.at(0);
}

NumberResult
handle_divide(const NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) / std::get<int>(parts.at(1));
}

NumberResult
handle_sum(const NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) + std::get<int>(parts.at(1));
}

NumberResult
handle_product(const NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) + std::get<int>(parts.at(1));
}

NumberResult
handle_minus(const NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) + std::get<int>(parts.at(1));
}

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
      {{"Space", "NumberRest"}, {"pass", {1}}},
    }},
    {"NumberRest", {
      {{scan_range('0', '9')}, {"digit", {0}}},
      {{"NumberRest", scan_range('0', '9')}, {"number", {0, 1}}},
    }},
    {"Space", {
      {{Epsilon()}},
      {{"Space", scan_char(' ')}},
      {{"Space", scan_char('\t')}},
      {{"Space", scan_char('\n')}},
    }},
    {"Sum", {
      {{"Product"}, {"pass", {0}}},
      {{"Sum", "Space", scan_char('+'), "Product"}, {"sum", {0, 3}}},
      {{"Sum", "Space", scan_char('-'), "Product"}, {"minus", {0, 3}}},
    }},
    {"Product", {
      {{"Facter"}, {"pass", {0}}},
      {{"Product", "Space", scan_char('*'), "Facter"}, {"product", {0, 3}}},
      {{"Product", "Space", scan_char('/'), "Facter"}, {"divide", {0, 3}}},
    }},
    {"Facter", {
      {{"Space", scan_char('('), "Sum", "Space", scan_char(')')}, {"pass", {2}}},
      {{"Number"}, {"pass", {0}}},
    }},
    {"Input", {
      {{"Sum", "Space"}, {"pass", {0}}},
    }},
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

  auto [success, elapsed, item_sets] =
    process_input(debug, ids["Input"], argv[1], grammar_rules);
  (void)item_sets;

  if (success)
  {
    std::unordered_map<std::string, earley::ActionResult<int>(*)(
      const std::vector<earley::ActionResult<int>>&
    )> actions;

    add_action("pass", actions, &handle_pass);
    add_action("digit", actions, &handle_digit);
    add_action("number", actions, &handle_number);
    add_action("sum", actions, &handle_sum);
    add_action("product", actions, &handle_product);
    add_action("divide", actions, &handle_divide);
    add_action("minus", actions, &handle_minus);

    auto value = earley::run_actions(ids["Input"], argv[1], actions, item_sets);
    std::cout << std::get<int>(value) << std::endl;
  }

  if (!success)
  {
    std::cerr << "Unable to parse" << std::endl;
  } else if (timing)
  {
    std::cout << elapsed << std::endl;
  }

  Grammar ebnf = {
    {"Grammar", {
      {{Epsilon()}},
      {{"Grammar", "Space", "Nonterminal"}},
    }},
    {"Space", {
      {{Epsilon()}},
      {{' '}},
      {{'\n'}},
      {{'\t'}},
    }},
    {"Nonterminal", {
      {{"Name", "Space", '-', '>', "Rules"}}
    }},
    {"Rules", {
      {{"Rules", "Space", '|', "Rule"}},
      {{"Rule"}},
    }},
    {"Rule", {
      {{Epsilon()}},
      {{"Rule", "Production"}},
    }},
    {"Production", {
      {{"Name"}},
      {{"Literal"}}
    }},
    {"Literal", {
      {{'\'', "Char", '\''}},
      {{'[', "Ranges", ']'}},
      {{'"', "Chars", '"'}},
    }},
    {"Name", {
      {{"Space", "NameStart", "NameRest"}},
    }},
    {"NameStart", {
      {{scan_range('a', 'z')}},
      {{scan_range('A', 'Z')}},
    }},
    {"NameRest", {
      {{Epsilon()}},
      {{"NameRest", "NameStart"}},
      {{"NameRest", scan_range('0', '9')}},
    }},
    {"Ranges", {
      {{"Range"}},
      {{"Ranges", "Range"}},
    }},
    {"Range", {
      {{scan_range('a', 'z'), '-', scan_range('a', 'z')}},
      {{scan_range('A', 'Z'), '-', scan_range('A', 'Z')}},
      {{scan_range('0', '9'), '-', scan_range('0', '9')}},
    }},
    {"Chars", {
      {{"Char"}},
      {{"Chars", "Char"}},
    }},
    {"Char", {
      {{scan_range('0', 'z')}},
      {{'('}},
      {{')'}},
    }},
  };

  if (argc < 3)
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

  auto [ebnf_parsed, ebnf_time, ebnf_items] =
    process_input(debug, ebnf_ids["Grammar"], argv[2], ebnf_rules);
  (void)ebnf_items;

  if (!ebnf_parsed)
  {
    std::cerr << "Unable to parse ebnf" << std::endl;
    exit(1);
  }
  else
  {
    std::cout << ebnf_time << std::endl;
  }

  return 0;
}
