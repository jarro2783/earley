#include "earley.hpp"
#include "numbers.hpp"

using namespace earley;

namespace
{
  earley::Grammar grammar = {
    {"Number", {
      {{"Space", "NumberRest"}, {"pass", {1}}},
    }},
    {"NumberRest", {
      {{scan_range('0', '9')}, {"digit", {0}}},
      {{"NumberRest", scan_range('0', '9')}, {"number", {0, 1}}},
    }},
    {"Space", {
      {{}},
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

  typedef earley::ActionResult<int> NumberResult;
  typedef std::vector<NumberResult> NumbersParts;

  NumberResult
  handle_digit(NumbersParts& parts)
  {
    if (parts.size() == 1)
    {
      auto& ch = parts[0];
      if (holds<char>(ch))
      {
        return get<char>(ch) - '0';
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

      if (holds<int>(integer) &&
          holds<char>(character))
      {
        value = get<int>(integer) * 10 + (get<char>(character) - '0');
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
  handle_divide(NumbersParts& parts)
  {
    return get<int>(parts.at(0)) / get<int>(parts.at(1));
  }

  NumberResult
  handle_sum(NumbersParts& parts)
  {
    return get<int>(parts.at(0)) + get<int>(parts.at(1));
  }

  NumberResult
  handle_product(NumbersParts& parts)
  {
    return get<int>(parts.at(0)) * get<int>(parts.at(1));
  }

  NumberResult
  handle_minus(NumbersParts& parts)
  {
    return get<int>(parts.at(0)) - get<int>(parts.at(1));
  }
}

void
parse_expression(const std::string& expression, bool debug, bool timing)
{
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
    process_input(debug, ids["Input"], expression, grammar_rules, ids);
  (void)item_sets;

  if (success)
  {
    if (timing)
    {
      std::cout << elapsed << std::endl;
    }

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
        pointers, ids["Input"], expression, actions, item_sets, ids);

    if (holds<int>(value))
    {
      std::cout << get<int>(value) << std::endl;
    }
    else
    {
      std::cout << "Failed to compute value" << std::endl;
    }
  }

}
