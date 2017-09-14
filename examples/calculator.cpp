#include <chrono>

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
get_tokens(const char* input)
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  rules.insert_macro("alpha", "[a-zA-Z]");
  rules.insert_macro("num", "[0-9]");

  rules.push("[1-9]{num}*", NUMBER);
  rules.push("{alpha}({alpha}|{num})*", IDENTIFIER);
  rules.push("[ \t]+", sm.skip());
  rules.push("\n", NEW_LINE);

  lexertl::memory_file file(input);

  if (file.data() == nullptr)
  {
    throw std::string("Unable to open ") + input;
  }

  lexertl::generator::build(rules, sm);
  lexertl::match_results<const char*> results(
    file.data(),
    file.data() + file.size());

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
    {"StatementList", {
      {{"Statement"}},
      {{"Statement", "StatementList"}},
    }},
    {"Statement", {
      {{"Expression"}},
      {{"IDENTIFIER", '=', "Expression"}},
    }},
    {"Expression", {
      {{"Product"}},
    }},
    {"Product", {
      {{"Product", '*', "Sum"}},
      {{"Product", '/', "Sum"}},
      {{"Sum"}},
    }},
    {"Sum", {
      {{"Sum", '+', "Facter"}},
      {{"Sum", '-', "Facter"}},
      {{"Facter"}},
    }},
  };
}

void
run_calculator(char* file)
{
  auto tokens = get_tokens(file);
  earley::fast::TerminalList terminals(tokens.begin(), tokens.end());
  auto grammar = make_grammar();

  earley::fast::grammar::Grammar built("StatementList", grammar, terminal_names);

  std::chrono::time_point<std::chrono::system_clock> start_time, end;
  start_time = std::chrono::system_clock::now();

  earley::fast::Parser parser(built);

  parser.parse_input(terminals);

  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start_time;

  std::cout << "Fast parser took "
    << std::chrono::duration_cast<std::chrono::microseconds>(
      elapsed_seconds).count()
    << " microseconds" << std::endl;

  //for (size_t i = 0; i <= terminals.size(); ++i)
  //{
  //  std::cout << "-- Set " << i << " --" << std::endl;
  //  parser.print_set(i);
  //}
}

int main(int argc, char** argv)
{
  if (argc > 1)
  {
    try
    {
      run_calculator(argv[1]);
    } catch (const std::string& e)
    {
      std::cerr << "Error parsing: " << e << std::endl;
      return 1;
    }
  }

  return 0;
}
