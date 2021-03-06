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

  options.parse(argc, argv);

  if (options.count("help"))
  {
    std::cout << options.help();
    exit(1);
  }

  bool debug = options.count("debug");
  bool timing = options.count("timing");
  bool slow = options.count("slow");

  if (options.count("expression"))
  {
    parse_expression(options["expression"].as<std::string>(), debug, timing);
  }

  if (argc < 2)
  {
    std::cerr << "Give me an ebnf to parse" << std::endl;
    exit(0);
  }

  std::string to_parse;
  if (argc >= 3)
  {
    to_parse = argv[2];
  }

  earley::parse_ebnf(argv[1], debug, timing, slow, to_parse);

  return 0;
}
