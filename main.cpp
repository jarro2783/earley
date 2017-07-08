#include "cxxopts.hpp"
#include "earley.hpp"
#include "grammar.hpp"

typedef earley::ActionResult<int> NumberResult;

template <typename T>
void
add_action(
  const std::string& name,
  std::unordered_map<std::string, T (*)(std::vector<T>&)>& actions,
  T (*fun)(std::vector<T>&)
)
{
  actions[name] = fun;
}

typedef std::vector<NumberResult> NumbersParts;

NumberResult
handle_digit(NumbersParts& parts)
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
handle_number(NumbersParts& parts)
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

template <typename Result>
Result
handle_pass(std::vector<Result>& parts)
{
  return parts.at(0);
}

NumberResult
handle_divide(NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) / std::get<int>(parts.at(1));
}

NumberResult
handle_sum(NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) + std::get<int>(parts.at(1));
}

NumberResult
handle_product(NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) * std::get<int>(parts.at(1));
}

NumberResult
handle_minus(NumbersParts& parts)
{
  return std::get<int>(parts.at(0)) - std::get<int>(parts.at(1));
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

  HashSet<Item> foo;
  foo.insert(Item(parens));

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

  auto [success, elapsed, item_sets, pointers] =
    process_input(debug, ids["Input"], argv[1], grammar_rules, ids);
  (void)item_sets;

  if (success)
  {
    std::unordered_map<std::string, earley::ActionResult<int>(*)(
      std::vector<earley::ActionResult<int>>&
    )> actions;

    add_action("pass", actions, &handle_pass);
    add_action("digit", actions, &handle_digit);
    add_action("number", actions, &handle_number);
    add_action("sum", actions, &handle_sum);
    add_action("product", actions, &handle_product);
    add_action("divide", actions, &handle_divide);
    add_action("minus", actions, &handle_minus);

    auto value = earley::run_actions(
        pointers, ids["Input"], argv[1], actions, item_sets);

    if (std::holds_alternative<int>(value))
    {
      std::cout << std::get<int>(value) << std::endl;
    }
    else
    {
      std::cout << "Failed to compute value" << std::endl;
    }
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
      {{"Name", "Space", '-', '>', "Rules"}}
    }},
    {"Rules", {
      {{"Rule"}, {"create_list", {0}}},
      {{"Rules", "Space", '|', "Rule"}, {"append_list", {0, 3}}},
    }},
    {"Rule", {
      {{Epsilon()}, {"create_list", {}}},
      {{"Productions"}, {"rule", {0}}},
      {{"Productions", "Action"}, {"rule", {0, 1}}},
    }},
    {"Productions", {
      {{"Production"}},
      {{"Productions", "Production"}, {"append_list", {0, 1}}},
    }},
    {"Production", {
      {{"Name"}},
      {{"Space", "Literal"}},
    }},
    {"Action", {
      {{"Space", '#', "Space", "Name"}},
    }},
    {"Literal", {
      {{'\'', "Char", '\''}},
      {{'[', "Ranges", ']'}},
      {{'"', "Chars", '"'}},
    }},
    {"Name", {
      {{"Space", "NameStart"}},
      {{"Space", "NameStart", "NameRest"}},
    }},
    {"NameStart", {
      {{scan_range('a', 'z')}, {"pass", {0}}},
      {{scan_range('A', 'Z')}, {"pass", {0}}},
    }},
    {"NameRest", {
      {{Epsilon()}, {"create_string", {}}},
      {{"NameRest", "NameStart"}, {"append_string", {0, 1}}},
      {{"NameRest", scan_range('0', '9')}, {"append_string", {0, 1}}},
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

  auto [ebnf_parsed, ebnf_time, ebnf_items, ebnf_pointers] =
    process_input(debug, ebnf_ids["Grammar"], argv[2], ebnf_rules, ebnf_ids);
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
    add_action("append_list", actions, &ast::action_append_list<std::vector<GrammarPtr>>);
    add_action("create_list", actions, &ast::action_create_list<std::vector<GrammarPtr>>);
    add_action("create_string", actions, &ast::action_create_list<std::string>);
    add_action("append_string", actions, &ast::action_append_list<std::string>);

    auto value = earley::run_actions(
        ebnf_pointers, ebnf_ids["Grammar"], argv[2], actions, ebnf_items);
  }

  return 0;
}
