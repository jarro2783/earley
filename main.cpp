#include "cxxopts.hpp"
#include "earley.hpp"

typedef earley::ActionResult<int> NumberResult;

template <typename T>
void
add_action(
  const std::string& name,
  std::vector<T (*)(const std::vector<T>&)>& actions,
  const std::unordered_map<std::string, size_t>& ids,
  T (*fun)(const std::vector<T>&)
)
{
  auto iter = ids.find(name);

  if (iter == ids.end())
  {
    std::cerr << "Rule not found" << std::endl;
    return;
  }

  auto pos = iter->second;
  if (actions.size() <= pos)
  {
    actions.resize(pos+1);
  }
  actions[pos] = fun;
}

NumberResult
handle_number(const std::vector<NumberResult>& parts)
{
  int value = 0;
  switch (parts.size())
  {
    case 1:
    {
      auto& ch = parts[0];
      if (std::holds_alternative<char>(ch))
      {
        value = std::get<char>(ch) - '0';
      }
      else
      {
        std::cerr << "Invalid type in handle_number[1]" << std::endl;
        return 0;
      }
    }
    break;

    case 2:
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
    break;

    default:
    std::cerr << "Invalid arguments to handle_number" << std::endl;
    return 0;
  }

  std::cout << "Parsed: " << value << std::endl;
  return value;
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
      {{"Space", "NumberRest"}, {1}},
    }},
    {"NumberRest", {
      {{scan_range('0', '9')}, {0}},
      {{"NumberRest", scan_range('0', '9')}, {0, 1}},
    }},
    {"Space", {
      {{Epsilon()}},
      {{"Space", scan_char(' ')}},
      {{"Space", scan_char('\t')}},
      {{"Space", scan_char('\n')}},
    }},
    {"Sum", {
      {{"Product"}},
      {{"Sum", "Space", scan_char('+'), "Product"}},
      {{"Sum", "Space", scan_char('-'), "Product"}},
    }},
    {"Product", {
      {{"Number"}},
      {{"Product", "Space", scan_char('*'), "Number"}},
      {{"Product", "Space", scan_char('/'), "Number"}},
    }},
    {"Input", {
      {{"Sum", "Space"}},
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
    std::vector<earley::ActionResult<int>(*)(
      const std::vector<earley::ActionResult<int>>&
    )> actions;

    add_action("NumberRest", actions, ids, &handle_number);
    earley::run_actions(ids["Input"], argv[1], actions, item_sets);
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
      {{"NameStart", "NameRest"}},
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
