#include <earley/fast.hpp>
#include <lexertl/generator.hpp>
#include <lexertl/lookup.hpp>
#include <lexertl/memory_file.hpp>

// TODO: this should be autogenerated by earley.
enum Tokens
{
  NUMBER = 256,
  IDENTIFIER,
  NEW_LINE,
};

namespace
{
  earley::fast::grammar::TerminalIndices terminal_names{
    {"IDENTIFIER", IDENTIFIER},
    {"NUMBER", NUMBER},
  };
}

std::vector<int>
get_tokens(const char* input, size_t length)
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  rules.insert_macro("alpha", "[a-zA-Z]");
  rules.insert_macro("num", "[0-9]");

  rules.push("[1-9]{num}*", NUMBER);
  rules.push("{alpha}({alpha}|{num})*", IDENTIFIER);
  rules.push("[ \t]+", sm.skip());
  rules.push("\n", NEW_LINE);

  lexertl::generator::build(rules, sm);
  lexertl::match_results<const char*> results(input, input + length);

  lexertl::lookup(sm, results);

  std::vector<int> tokens;

  int lines = 1;
  while (results.id != 0)
  {
    if (results.id == NEW_LINE)
    {
      ++lines;
    }
    else
    {
      std::cout << "Id: " << static_cast<int>(results.id) << ", Token: '" <<
        results.str () << "': " << std::boolalpha << results.bol << "\n";

      //TODO: this assumes ASCII
      if (results.id == results.npos())
      {
        tokens.push_back(*results.first);
      }
      else
      {
        tokens.push_back(results.id);
      }
    }

    lexertl::lookup(sm, results);
  }

  std::cout << "Lines: " << lines << std::endl;

  return tokens;
}

earley::Grammar
make_grammar()
{
  return {
    {"Facter", {
      {{'(', "Expression", ')'}},
      {{"NUMBER"}},
      {{"IDENTIFIER"}}
    }},
    {"Expression", {
      {{"Sum"}},
    }},
    {"Sum", {
      {{"Sum", '+', "Facter"}},
      {{"Facter"}},
    }},
  };
}

int main(int argc, char** argv)
{
  if (argc > 1)
  {
    auto tokens = get_tokens(argv[1], strlen(argv[1]));
    earley::fast::TerminalList terminals(tokens.begin(), tokens.end());
    auto grammar = make_grammar();

    earley::fast::grammar::Grammar built("Expression", grammar, terminal_names);
    earley::fast::Parser parser(built);

    std::cout << "-- Set " << 0 << " --" << std::endl;
    parser.print_set(0);
    for (size_t i = 0; i != terminals.size(); ++i)
    {
      parser.parse(terminals, i);
      std::cout << "-- Set " << i+1 << " --" << std::endl;
      parser.print_set(i+1);
    }
  }
}
