#include <earley/fast.hpp>
#include "earley.hpp"
#include "grammar.hpp"
#include <lexertl/memory_file.hpp>
#include <sstream>
#include <fstream>

namespace
{
  void
  generate_grammar(const char*, const char*, const char*);

  void
  write_grammar(
    const char*,
    const earley::Grammar&,
    const earley::TerminalMap&,
    const std::string&
  );

  void
  print_node(std::ostream& os, const earley::Production& node);

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
    std::cerr << "Error: " << c << std::endl;
    return 1;
  } catch (const std::string& s)
  {
    std::cerr << "Error: " << s << std::endl;
  }
  catch (const Terminate&)
  {
    return 1;
  }

  return 0;
}

namespace
{

void
generate_grammar(const char* prefix, const char* input, const char* output)
{
  lexertl::memory_file raw(input);

  if (raw.data() == 0)
  {
    throw std::string("Unable to open ") + input;
  }

  std::string text(raw.data(), raw.data() + raw.size());

  std::cout << "Building grammar" << std::endl;
  auto [grammar, terminals, start] = earley::parse_grammar(text);

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

  write_grammar(prefix, grammar, terminals, output);
}

void
write_grammar(
  const char* prefix,
  const earley::Grammar& grammar,
  const earley::TerminalMap& terminals,
  const std::string& output_prefix
)
{
  std::ostringstream os;
  std::ostringstream tokens;

  os << "#include \"" << output_prefix << ".hpp\"\n\n";
  os << "earley::TerminalMap " << prefix << "_terminals = {\n";

  tokens << "enum " << prefix << "_Tokens {\n";

  for (auto& [symbol, index]: terminals)
  {
    os << "  {\"" << symbol << "\", " << index << "},\n";
    tokens << "  " << symbol << " = " << index << ",\n";
  }

  os << "};\n\n";
  tokens << "};\n";

  os << "earley::Grammar " << prefix << "_grammar{\n";
  for (auto& [name, rules]: grammar)
  {
    os << "  {\"" << name << "\", {\n";
    for (auto& rule: rules)
    {
      os << "    {{";
      for (auto& node: rule.productions())
      {
        print_node(os, node);
        os << ", ";
      }
      os << "}},\n";
    }
    os << "  }},\n";
  }
  os << "};\n";

  std::ofstream of((output_prefix + ".cpp").c_str());
  of << os.str();

  std::ofstream header((output_prefix + ".hpp").c_str());
  header << "#include<earley.hpp>\n\n"
         << "extern earley::TerminalMap " << prefix << "_terminals;\n"
         << "extern earley::Grammar " << prefix << "_grammar;\n\n"
         << tokens.str();
}

struct NodePrinter
{
  NodePrinter(std::ostream& os)
  : m_os(os)
  {
  }

  void
  operator()(const std::string& s)
  {
    m_os << '"' << s << '"';
  }

  void
  operator()(char c)
  {
    // TODO: escape
    m_os << "'" << c << "'";
  }

  void
  operator()(const earley::Scanner&)
  {
    throw "Unsupported";
  }

  private:
  std::ostream& m_os;
};

void
print_node(std::ostream& os, const earley::Production& node)
{
  NodePrinter printer(os);
  std::visit(printer, node);
}

}
