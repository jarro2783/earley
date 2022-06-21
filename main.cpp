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
    ("slow", "Run the slow parser")
  ;

  auto result = options.parse(argc, argv);

  if (result.count("help"))
  {
    std::cout << options.help();
    exit(1);
  }

  bool debug = result.count("debug");
  bool timing = result.count("timing");
  bool slow = result.count("slow");

  if (result.count("expression"))
  {
    parse_expression(result["expression"].as<std::string>(), debug, timing);
  }

  if (argc < 2)
  {
    std::cerr << "Give me an ebnf to parse" << std::endl;
    exit(0);
  }

  auto extras = result.unmatched();

  std::string to_parse;
  if (extras.size() > 1)
  {
    to_parse = extras[0];
  }

  earley::parse_ebnf(argv[1], debug, timing, slow, to_parse);

  return 0;
}
