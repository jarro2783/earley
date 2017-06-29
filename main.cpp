#include "cxxopts.hpp"
#include "earley.hpp"

int main(int argc, char** argv)
{
  using namespace earley;

  Item parens(0, {'(', 0ul, ')'});
  Item empty(0, {earley::Epsilon()});

  std::unordered_map<size_t, ItemSet> rules{
    {0ul, {parens, empty,}},
  };

  process_input(0, "()", rules);
  process_input(0, "(())", rules);
  process_input(0, "((", rules);

  // number -> space | number [0-9]
  // sum -> product | sum + product
  // product -> number | product * number
  // space -> | space [ \t\n]
  // input -> sum space
  std::unordered_map<size_t, ItemSet> numbers{
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
      {"Space"},
      {"Number", '0'},
      {"Number", '1'},
      {"Number", '2'},
      {"Number", '3'},
      {"Number", '4'},
      {"Number", '5'},
      {"Number", '6'},
      {"Number", '7'},
      {"Number", '8'},
      {"Number", '9'},
    }},
    {"Space", {
      {Epsilon()},
      {"Space", ' '},
      {"Space", '\t'},
      {"Space", '\n'},
    }},
    {"Sum", {
      {"Product"},
      {"Sum", "Space", '+', "Product"},
      {"Sum", "Space", '-', "Product"},
    }},
    {"Product", {
      {"Number"},
      {"Product", "Space", '*', "Number"},
      {"Product", "Space", '/', "Number"},
    }},
    {"Input", {{"Sum", "Space"}}},
  };

  if (argc < 2)
  {
    std::cerr << "Give me an expression to parse" << std::endl;
    exit(1);
  }

  auto [grammar_rules, ids] = generate_rules(grammar);

  std::cout << "Generated grammar rules:" << std::endl;
  for (auto& rule : grammar_rules)
  {
    std::cout << rule.first << ":" << std::endl;
    for (auto& item : rule.second)
    {
      std::cout << item << std::endl;
    }
  }

  //process_input(4, argv[1], numbers);
  process_input(ids["Input"], argv[1], grammar_rules);

  return 0;
}
