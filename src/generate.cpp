#include <earley/fast.hpp>
#include "grammar.hpp"
#include <lexertl/memory_file.hpp>

namespace
{
  void
  generate_grammar(const char*, const char*, const char*);

  struct Terminate
  {
  };
}

int main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " prefix input output" << std::endl;
    return 1;
  }

  try
  {
    generate_grammar(argv[1], argv[2], argv[3]);
  } catch (const char* c)
  {
    std::cerr << "Failed to parse file: " << c << std::endl;
    return 1;
  } catch (const Terminate&)
  {
    return 1;
  }

  return 0;
}

void
generate_grammar(const char* prefix, const char* input, const char* output)
{
  lexertl::memory_file raw(input);

  if (raw.data() == 0)
  {
    throw "Unable to open grammar/c_raw";
  }

  std::string text(raw.data(), raw.data() + raw.size());

  std::cout << "Building grammar" << std::endl;
  auto [grammar, start] = earley::parse_grammar(text);

  earley::fast::grammar::Grammar built(start, grammar, terminals);

  auto valid = built.validate();

  if (!valid.is_valid())
  {
    std::cerr << "Error: grammar is not valid\n"
      << "-- Undefined rules --\n";

    for (auto& rule: valid.undefined())
    {
      std::cerr << rule << "\n";
    }

    std::cerr << std::endl;
    throw Terminate();
  }
}