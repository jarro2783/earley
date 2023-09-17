#include "cxxopts.hpp"
#include "earley.hpp"
#include "grammar.hpp"
#include "numbers.hpp"

auto parse_options(cxxopts::Options& options, int argc, char** argv)
{
  try {
    return options.parse(argc, argv);
  } catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  }

}

int main(int argc, char** argv)
{
  using namespace earley;

  cxxopts::Options options("earley", "an earley parser");
  options.add_options()
    ("d,debug", "turn on debugging")
    ("expression", "parse a simple expression", cxxopts::value<std::string>())
    ("h,help", "show help")
    ("t,timing", "print timing")
    ("slow", "Run the slow parser")
  ;

  auto result = parse_options(options, argc, argv);

  if (result.count("help"))
  {
    std::cout << options.help();
    return 0;
  }

  bool debug = result.count("debug");
  bool timing = result.count("timing");
  bool slow = result.count("slow");

  if (result.count("expression"))
  {
    parse_expression(result["expression"].as<std::string>(), debug, timing);
    return 0;
  }

  if (argc < 2)
  {
    std::cerr << "Give me an ebnf to parse" << std::endl;
    exit(1);
  }

  auto extras = result.unmatched();

  std::string to_parse;
  if (extras.size() > 1)
  {
    to_parse = extras[1];
  }

  try {
    earley::parse_ebnf(extras[0], debug, timing, slow, to_parse);
  }
  catch (const earley::ast::InvalidGrammar& e)
  {
    std::cerr << "Invalid grammar, exiting" << std::endl;
    return 1;
  }

  return 0;
}
