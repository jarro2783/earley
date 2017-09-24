#include <chrono>

#include <earley/fast.hpp>
#include "grammar.hpp"
#include <lexertl/generator.hpp>
#include <lexertl/lookup.hpp>
#include <lexertl/memory_file.hpp>

namespace
{
  enum Token
  {
    // Words
    AUTO = 256,
    BREAK,
    CASE,
    CHAR,
    CONST, // 260
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    EXTERN,
    FLOAT,
    FOR,
    IF,
    INT, // 270
    LONG,
    RESTRICT,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STRUCT,
    TYPEDEF,
    UNION,
    UNSIGNED, // 280
    VOID,
    VOLATILE,
    WHILE,

    _BOOL,
    _COMPLEX,
    _IMAGINARY,

    // Symbols
    EQ_OP,
    RIGHT_OP,
    LEFT_OP,
    INC_OP, // 290
    DEC_OP,
    PTR_OP,

    // Others
    IDENTIFIER,
    CONSTANT,
    STRING_LITERAL,
  };

  earley::fast::grammar::TerminalIndices terminals
  {
    {"_BOOL", Token::_BOOL},
    {"_COMPLEX", Token::_COMPLEX},
    {"_IMAGINARY", Token::_IMAGINARY},
    {"AUTO", Token::AUTO},
    {"BREAK", Token::BREAK},
    {"CASE", Token::CASE},
    {"CHAR", Token::CHAR},
    {"CONST", Token::CONST},
    {"CONSTANT", Token::CONSTANT},
    {"CONTINUE", Token::CONTINUE},
    {"DEC_OP", Token::DEC_OP},
    {"DEFAULT", Token::DEFAULT},
    {"DOUBLE", Token::DOUBLE},
    {"ENUM", Token::ENUM},
    {"EXTERN", Token::EXTERN},
    {"FLOAT", Token::FLOAT},
    {"IDENTIFIER", Token::IDENTIFIER},
    {"IF", Token::IF},
    {"INC_OP", Token::INC_OP},
    {"INT", Token::INT},
    {"LEFT_OP", Token::LEFT_OP},
    {"LONG", Token::LONG},
    {"PTR_OP", Token::PTR_OP},
    {"RESTRICT", Token::RESTRICT},
    {"RETURN", Token::RETURN},
    {"RIGHT_OP", Token::RIGHT_OP},
    {"SHORT", Token::SHORT},
    {"SIGNED", Token::SIGNED},
    {"SIZEOF", Token::SIZEOF},
    {"STRUCT", Token::STRUCT},
    {"TYPEDEF", Token::TYPEDEF},
    {"UNION", Token::UNION},
    {"UNSIGNED", Token::UNSIGNED},
    {"VOID", Token::VOID},
    {"VOLATILE", Token::VOLATILE},
    {"WHILE", Token::WHILE},

    {"EQ_OP", Token::EQ_OP},
    {"RIGHT_OP", Token::RIGHT_OP},

    {"STRING_LITERAL", Token::STRING_LITERAL},
  };
}

std::vector<int>
get_tokens(const char* file)
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  std::initializer_list<std::pair<const char*, Token>> symbols = {
    {"_Bool", Token::_BOOL},
    {"_Complex", Token::_COMPLEX},
    {"_Imaginary", Token::_IMAGINARY},
    {"auto", Token::AUTO},
    {"break", Token::BREAK},
    {"case", Token::CASE},
    {"char", Token::CHAR},
    {"const", Token::CONST},
    {"continue", Token::CONTINUE},
    {"default", Token::DEFAULT},
    {"do", Token::DO},
    {"double", Token::DOUBLE},
    {"else", Token::ELSE},
    {"enum", Token::ENUM},
    {"extern", Token::EXTERN},
    {"float", Token::FLOAT},
    {"for", Token::FOR},
    {"if", Token::IF},
    {"int", Token::INT},
    {"long", Token::LONG},
    {"restrict", Token::RESTRICT},
    {"return", Token::RETURN},
    {"signed", Token::SIGNED},
    {"sizeof", Token::SIZEOF},
    {"struct", Token::STRUCT},
    {"union", Token::UNION},
    {"unsigned", Token::UNSIGNED},
    {"typedef", Token::TYPEDEF},
    {"void", Token::VOID},
    {"volatile", Token::VOLATILE},

    // symbols
    {"<<", Token::LEFT_OP},
    {">>", Token::RIGHT_OP},
    {"==", Token::EQ_OP},
    {"\\+\\+", Token::INC_OP},
    {"--", Token::DEC_OP},
    {"->", Token::PTR_OP},
  };

  std::vector<int> characters{
    '=',
    ':',
    ';',
    ',',
    '!',
    '&',
    '~',
    '>',
    '<',
    '-',
    '%',
  };

  for (auto& [text, token] : symbols) {
    rules.push(text, static_cast<int>(token));
  }

  for (auto c : characters)
  {
    rules.push(std::string(1, c), c);
  }

  rules.insert_macro("WS", "[ \\t\\v\\n\\f]");

  rules.insert_macro("O", "[0-7]");
  rules.insert_macro("D", "[0-9]");
  rules.insert_macro("L", "[A-Za-z_]");
  rules.insert_macro("A", "[A-Za-z_0-9]");
  rules.insert_macro("NZ", "[1-9]");
  rules.insert_macro("H", "[a-fA-F0-9]");
  rules.insert_macro("HP", "0[xX]");
  rules.insert_macro("E", "[Ee][+-]?{D}+");
  rules.insert_macro("FS", "f|F|l|L");
  rules.insert_macro("IS", "(((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))");

  rules.insert_macro("CP", "u|U|L");
  rules.insert_macro("SP", "(u8|u|U|L)");
  rules.insert_macro("ES", R"*((\\(['"?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+)))*");

  rules.push("\\/", '/');
  rules.push("\\.", '.');
  rules.push("\\(", '(');
  rules.push("\\)", ')');
  rules.push("\\+", '+');
  rules.push("\\*", '*');
  rules.push("\\{", '{');
  rules.push("\\}", '}');
  rules.push("\\[", '[');
  rules.push("\\]", ']');
  rules.push("\\>", '>');
  rules.push("\\<", '<');
  rules.push("\\^", '^');
  rules.push("\\?", '?');
  rules.push("\\|", '|');

  rules.push("{L}{A}*", Token::IDENTIFIER);

  // integers
  rules.push("{NZ}{D}*", Token::CONSTANT);
  rules.push("{HP}{H}+{IS}?", Token::CONSTANT);
  rules.push("0{O}*{IS}?", Token::CONSTANT);

  // character
  rules.push(R"**({CP}?'([^'\\\n]|{ES})+')**", Token::CONSTANT);

  // floats
  rules.push("{D}+{E}{FS}?", Token::CONSTANT);
  rules.push("D*\\.{D}+{E}?{FS}?", Token::CONSTANT);

  // string
  rules.push(R"**(({SP}?["]([^\\"\n\r]|{ES})*\"{WS}*)+)**", Token::STRING_LITERAL);

  rules.push("{WS}", sm.skip());

  lexertl::memory_file mf(file);

  if (mf.data() == nullptr)
  {
    throw std::string("Unable to open ") + file;
  }

  lexertl::generator::build(rules, sm);
  lexertl::match_results<const char*> results(
    mf.data(),
    mf.data() + mf.size()
  );

  lexertl::lookup(sm, results);

  std::vector<int> tokens;

  while (results.id != 0)
  {
    if (results.id == results.npos())
    {
      int i = 0;
      std::cerr << "Last few tokens are: ";
      for (auto iter = tokens.rbegin(); iter != tokens.rend() && i < 10; ++iter, ++i)
      {
        std::cerr << *iter << " ";
      }
      std::cerr << std::endl;

      std::ostringstream ss;
      ss << "Unrecognised token starting with " <<
        std::string(
          results.first,
          std::min(results.first + 30, mf.data() + mf.size()))
         << " at position " << (results.first - mf.data());
      throw ss.str();
    }
    else
    {
#if 0
      std::cerr << tokens.size() << ": ";
      for (auto c = results.first; c != results.second; ++c)
      {
        std::cerr << *c;
      }
      std::cerr << std::endl;
#endif

      tokens.push_back(results.id);
    }

    lexertl::lookup(sm, results);
  }

  return tokens;
}

void
parse_c(const char* file)
{
  lexertl::memory_file c_bnf("grammar/c_raw");

  if (c_bnf.data() == 0)
  {
    throw "Unable to open grammar/c_raw";
  }

  std::string c_definition(c_bnf.data(), c_bnf.data() + c_bnf.size());

  auto [grammar, start] = earley::parse_grammar(c_definition);
  auto tokens = get_tokens(file);
  earley::fast::TerminalList symbols(tokens.begin(), tokens.end());
  earley::fast::grammar::Grammar built(start, grammar, terminals);

  earley::fast::Parser parser(built);

  size_t i;
  try {
    for (i = 0; i != symbols.size(); ++i)
    {
      parser.parse(symbols, i);
    }
  } catch(...)
  {
    for (size_t j = 0; j <= i; ++j)
    {
      std::cout << "-- Set " << j << " --" << std::endl;
      std::cout << "Symbol: " << tokens[j] << std::endl;
      parser.print_set(j);
    }
    throw;
  }
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Please provide a C file" << std::endl;
    return 1;
  }

  try
  {
    parse_c(argv[1]);
  }
  catch (const char* c)
  {
    std::cerr << "Error parsing: " << c << std::endl;
    return 1;
  }
  catch (const std::string& s)
  {
    std::cerr << "Error parsing: " << s << std::endl;
    return 1;
  }

  return 0;
}
