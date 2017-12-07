#include <chrono>

#include <earley/fast.hpp>
#include "earley/timer.hpp"
#include "grammar.hpp"
#include <lexertl/generator.hpp>
#include <lexertl/lookup.hpp>
#include <lexertl/memory_file.hpp>

#include "../c_grammar.hpp"

namespace
{
  class Terminate {};

  struct Position
  {
    int line;
    int column;
  };

  std::vector<Position> positions;

  #if 0
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
    GOTO, // 270
    IF,
    INT,
    LONG,
    REGISTER,
    RESTRICT,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC, // 280
    STRUCT,
    SWITCH,
    TYPEDEF,
    UNION,
    UNSIGNED,
    VOID,
    VOLATILE,
    WHILE,

    _BOOL,
    _COMPLEX, // 290
    _IMAGINARY,

    // Symbols
    ADD_ASSIGN,
    AND_ASSIGN,
    AND_OP,
    DEC_OP,
    DIV_ASSIGN,
    EQ_OP,
    ELIPSIS,
    GE_OP,
    INC_OP, // 300
    INLINE,
    LE_OP,
    LEFT_ASSIGN,
    LEFT_OP,
    MOD_ASSIGN,
    MUL_ASSIGN,
    NE_OP,
    OR_ASSIGN,
    OR_OP,
    PTR_OP, // 310
    RIGHT_ASSIGN,
    RIGHT_OP,
    SUB_ASSIGN,
    XOR_ASSIGN,

    // Others
    IDENTIFIER,
    CONSTANT,
    STRING_LITERAL,

    SPACE,
  };
#endif

  earley::fast::grammar::TerminalIndices terminals
  {
    {"_BOOL", c_Tokens::_BOOL},
    {"_COMPLEX", c_Tokens::_COMPLEX},
    {"_IMAGINARY", c_Tokens::_IMAGINARY},
    {"ADD_ASSIGN", c_Tokens::ADD_ASSIGN},
    {"AND_ASSIGN", c_Tokens::AND_ASSIGN},
    {"AUTO", c_Tokens::AUTO},
    {"BREAK", c_Tokens::BREAK},
    {"CASE", c_Tokens::CASE},
    {"CHAR", c_Tokens::CHAR},
    {"CONST", c_Tokens::CONST},
    {"CONSTANT", c_Tokens::CONSTANT},
    {"CONTINUE", c_Tokens::CONTINUE},
    {"DEFAULT", c_Tokens::DEFAULT},
    {"DO", c_Tokens::DO},
    {"DOUBLE", c_Tokens::DOUBLE},
    {"ELIPSIS", c_Tokens::ELIPSIS},
    {"ELSE", c_Tokens::ELSE},
    {"ENUM", c_Tokens::ENUM},
    {"EXTERN", c_Tokens::EXTERN},
    {"FLOAT", c_Tokens::FLOAT},
    {"FOR", c_Tokens::FOR},
    {"GOTO", c_Tokens::GOTO},
    {"IDENTIFIER", c_Tokens::IDENTIFIER},
    {"IF", c_Tokens::IF},
    {"INLINE", c_Tokens::INLINE},
    {"INT", c_Tokens::INT},
    {"LONG", c_Tokens::LONG},
    {"REGISTER", c_Tokens::REGISTER},
    {"RESTRICT", c_Tokens::RESTRICT},
    {"RETURN", c_Tokens::RETURN},
    {"SHORT", c_Tokens::SHORT},
    {"SIGNED", c_Tokens::SIGNED},
    {"SIZEOF", c_Tokens::SIZEOF},
    {"STATIC", c_Tokens::STATIC},
    {"STRUCT", c_Tokens::STRUCT},
    {"SWITCH", c_Tokens::SWITCH},
    {"TYPEDEF", c_Tokens::TYPEDEF},
    {"UNION", c_Tokens::UNION},
    {"UNSIGNED", c_Tokens::UNSIGNED},
    {"VOID", c_Tokens::VOID},
    {"VOLATILE", c_Tokens::VOLATILE},
    {"WHILE", c_Tokens::WHILE},

    {"AND_OP", c_Tokens::AND_OP},
    {"DEC_OP", c_Tokens::DEC_OP},
    {"DIV_ASSIGN", c_Tokens::DIV_ASSIGN},
    {"EQ_OP", c_Tokens::EQ_OP},
    {"GE_OP", c_Tokens::GE_OP},
    {"INC_OP", c_Tokens::INC_OP},
    {"LE_OP", c_Tokens::LE_OP},
    {"LEFT_ASSIGN", c_Tokens::LEFT_ASSIGN},
    {"LEFT_OP", c_Tokens::LEFT_OP},
    {"NE_OP", c_Tokens::NE_OP},
    {"OR_OP", c_Tokens::OR_OP},
    {"PTR_OP", c_Tokens::PTR_OP},
    {"MOD_ASSIGN", c_Tokens::MOD_ASSIGN},
    {"MUL_ASSIGN", c_Tokens::MUL_ASSIGN},
    {"OR_ASSIGN", c_Tokens::OR_ASSIGN},
    {"RIGHT_ASSIGN", c_Tokens::RIGHT_ASSIGN},
    {"RIGHT_OP", c_Tokens::RIGHT_OP},
    {"SUB_ASSIGN", c_Tokens::SUB_ASSIGN},
    {"XOR_ASSIGN", c_Tokens::XOR_ASSIGN},

    {"STRING_LITERAL", c_Tokens::STRING_LITERAL},
  };
}

std::vector<int>
get_tokens(const char* file)
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  std::initializer_list<std::pair<const char*, c_Tokens>> symbols = {
    {"_Bool", c_Tokens::_BOOL},
    {"_Complex", c_Tokens::_COMPLEX},
    {"_Imaginary", c_Tokens::_IMAGINARY},
    {"auto", c_Tokens::AUTO},
    {"break", c_Tokens::BREAK},
    {"case", c_Tokens::CASE},
    {"char", c_Tokens::CHAR},
    {"const", c_Tokens::CONST},
    {"continue", c_Tokens::CONTINUE},
    {"default", c_Tokens::DEFAULT},
    {"do", c_Tokens::DO},
    {"double", c_Tokens::DOUBLE},
    {"else", c_Tokens::ELSE},
    {"enum", c_Tokens::ENUM},
    {"extern", c_Tokens::EXTERN},
    {"float", c_Tokens::FLOAT},
    {"for", c_Tokens::FOR},
    {"if", c_Tokens::IF},
    {"goto", c_Tokens::GOTO},
    {"inline", c_Tokens::INLINE},
    {"int", c_Tokens::INT},
    {"long", c_Tokens::LONG},
    {"register", c_Tokens::REGISTER},
    {"restrict", c_Tokens::RESTRICT},
    {"return", c_Tokens::RETURN},
    {"signed", c_Tokens::SIGNED},
    {"sizeof", c_Tokens::SIZEOF},
    {"short", c_Tokens::SHORT},
    {"static", c_Tokens::STATIC},
    {"struct", c_Tokens::STRUCT},
    {"switch", c_Tokens::SWITCH},
    {"union", c_Tokens::UNION},
    {"unsigned", c_Tokens::UNSIGNED},
    {"typedef", c_Tokens::TYPEDEF},
    {"void", c_Tokens::VOID},
    {"volatile", c_Tokens::VOLATILE},
    {"while", c_Tokens::WHILE},

    // symbols
    {R"("+=")", c_Tokens::ADD_ASSIGN},
    {"&=", c_Tokens::AND_ASSIGN},
    {"&&", c_Tokens::AND_OP},
    {"--", c_Tokens::DEC_OP},
    {R"("/=")", c_Tokens::DIV_ASSIGN},
    {R"("...")", c_Tokens::ELIPSIS},
    {"==", c_Tokens::EQ_OP},
    {">=", c_Tokens::GE_OP},
    {R"("++")", c_Tokens::INC_OP},
    {"<=", c_Tokens::LE_OP},
    {"<<=", c_Tokens::LEFT_ASSIGN},
    {"<<", c_Tokens::LEFT_OP},
    {"%=", c_Tokens::MOD_ASSIGN},
    {R"("*=")", c_Tokens::MUL_ASSIGN},
    {"!=", c_Tokens::NE_OP},
    {R"("|=")", c_Tokens::OR_ASSIGN},
    {R"("||")", c_Tokens::OR_OP},
    {"->", c_Tokens::PTR_OP},
    {">>=", c_Tokens::RIGHT_ASSIGN},
    {">>", c_Tokens::RIGHT_OP},
    {"-=", c_Tokens::SUB_ASSIGN},
    {R"("^=")", c_Tokens::XOR_ASSIGN},
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

  // integers
  rules.push("{NZ}{D}*{IS}?", c_Tokens::CONSTANT);
  rules.push("{HP}{H}+{IS}?", c_Tokens::CONSTANT);
  rules.push("0{O}*{IS}?", c_Tokens::CONSTANT);

  // character
  rules.push(R"**({CP}?'([^'\\\n]|{ES})+')**", c_Tokens::CONSTANT);

  // floats
  rules.push("{D}+{E}{FS}?", c_Tokens::CONSTANT);
  rules.push("{D}*\\.{D}+{E}?{FS}?", c_Tokens::CONSTANT);

  // string
  rules.push(R"**(({SP}?["]([^\\"\n\r]|{ES})*\"{WS}*)+)**", c_Tokens::STRING_LITERAL);

  rules.push("{WS}", c_Tokens::SPACE);

  rules.push("{L}{A}*", c_Tokens::IDENTIFIER);

  lexertl::memory_file mf(file);

  if (mf.data() == nullptr)
  {
    throw std::string("Unable to open ") + file;
  }

  std::cout << "Building lexer" << std::endl;
  lexertl::generator::build(rules, sm);

  std::cout << "Reading tokens" << std::endl;
  lexertl::match_results<const char*> results(
    mf.data(),
    mf.data() + mf.size()
  );

  lexertl::lookup(sm, results);

  std::vector<int> tokens;

  int line = 1;
  int column = 1;

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

      if (results.id != c_Tokens::SPACE)
      {
        positions.push_back({line, column});
        tokens.push_back(results.id);
      }

      for (auto pos = results.first; pos != results.second; ++pos)
      {
        if (*pos == '\n')
        {
          ++line;
          column = 0;
        }
        ++column;
      }
    }

    lexertl::lookup(sm, results);
  }

  return tokens;
}

void
parse_c(const char* file, bool dump)
{
#if 0
  lexertl::memory_file c_bnf("grammar/c_raw");

  if (c_bnf.data() == 0)
  {
    throw "Unable to open grammar/c_raw";
  }

  std::string c_definition(c_bnf.data(), c_bnf.data() + c_bnf.size());

  std::cout << "Building grammar" << std::endl;
  auto [grammar, g_terminals, start] = earley::parse_grammar(c_definition);
#endif
  auto memstart = static_cast<char*>(sbrk(0));

  std::cout << "Reading tokens" << std::endl;

  earley::Timer scan_timer;
  auto tokens = get_tokens(file);
  std::cout << "Scanner took " << scan_timer.count<std::chrono::microseconds>()
            << " microseconds" << std::endl
            << "Used " << (static_cast<char*>(sbrk(0)) - memstart)/1024
            << "kb of memory" << std::endl;


  std::cout << "Building grammar" << std::endl;
  earley::fast::TerminalList symbols(tokens.begin(), tokens.end());
  earley::fast::grammar::Grammar built("start", ::c_grammar, ::c_terminals);

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

  std::cout << "Parsing " << symbols.size() << " tokens" << std::endl;
  earley::fast::Parser parser(built, symbols);

  size_t i;
  try {
    earley::Timer timer;
    for (i = 0; i != symbols.size(); ++i)
    {
      parser.parse(i);
    }
    std::cout << "Parsing took " << timer.count<std::chrono::microseconds>()
      << " microseconds" << std::endl;

    auto memend = sbrk(0);

    std::cout << "Used " 
      << (static_cast<char*>(memend) - static_cast<char*>(memstart)) / 1024
      << "kb of memory" << std::endl;

    std::cout << "creating reductions" << std::endl;
    parser.create_reductions();
    std::cout << "done" << std::endl;

    parser.print_stats();
    std::cout << "HashTable collisions "
              << earley::hashtable_collisions << std::endl;

    //throw "foo";
  } catch(...)
  {
    auto& position = positions[i];
    std::cout << "Error at: " << position.line << ":" << position.column << std::endl;
    if (dump)
    {
      for (size_t j = 0; j <= i; ++j)
      {
        std::cout << "-- Set " << j << " --" << std::endl;
        if (j > 0)
        {
          std::cout << "Symbol: " << tokens[j-1] << std::endl;
        }
        parser.print_set(j);
      }
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
    parse_c(argv[1], true);
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
  catch (const Terminate&)
  {
    std::cerr << "exiting" << std::endl;
    return 1;
  }

  return 0;
}
