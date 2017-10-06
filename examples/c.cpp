#include <chrono>

#include <earley/fast.hpp>
#include "grammar.hpp"
#include <lexertl/generator.hpp>
#include <lexertl/lookup.hpp>
#include <lexertl/memory_file.hpp>

namespace
{
  class Terminate {};

  class Timer
  {
    public:

    Timer()
    : m_start(std::chrono::system_clock::now())
    {
    }

    auto
    now() const
    {
      return std::chrono::system_clock::now();
    }

    template <typename T>
    double
    count() const
    {
      return std::chrono::duration_cast<T>(
        (now() - m_start)).count();
    }

    private:
    std::chrono::time_point<std::chrono::system_clock> m_start;
  };

  struct Position
  {
    int line;
    int column;
  };

  std::vector<Position> positions;

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

  earley::fast::grammar::TerminalIndices terminals
  {
    {"_BOOL", Token::_BOOL},
    {"_COMPLEX", Token::_COMPLEX},
    {"_IMAGINARY", Token::_IMAGINARY},
    {"ADD_ASSIGN", Token::ADD_ASSIGN},
    {"AND_ASSIGN", Token::AND_ASSIGN},
    {"AUTO", Token::AUTO},
    {"BREAK", Token::BREAK},
    {"CASE", Token::CASE},
    {"CHAR", Token::CHAR},
    {"CONST", Token::CONST},
    {"CONSTANT", Token::CONSTANT},
    {"CONTINUE", Token::CONTINUE},
    {"DEFAULT", Token::DEFAULT},
    {"DO", Token::DO},
    {"DOUBLE", Token::DOUBLE},
    {"ELIPSIS", Token::ELIPSIS},
    {"ELSE", Token::ELSE},
    {"ENUM", Token::ENUM},
    {"EXTERN", Token::EXTERN},
    {"FLOAT", Token::FLOAT},
    {"FOR", Token::FOR},
    {"GOTO", Token::GOTO},
    {"IDENTIFIER", Token::IDENTIFIER},
    {"IF", Token::IF},
    {"INLINE", Token::INLINE},
    {"INT", Token::INT},
    {"LONG", Token::LONG},
    {"REGISTER", Token::REGISTER},
    {"RESTRICT", Token::RESTRICT},
    {"RETURN", Token::RETURN},
    {"SHORT", Token::SHORT},
    {"SIGNED", Token::SIGNED},
    {"SIZEOF", Token::SIZEOF},
    {"STATIC", Token::STATIC},
    {"STRUCT", Token::STRUCT},
    {"SWITCH", Token::SWITCH},
    {"TYPEDEF", Token::TYPEDEF},
    {"UNION", Token::UNION},
    {"UNSIGNED", Token::UNSIGNED},
    {"VOID", Token::VOID},
    {"VOLATILE", Token::VOLATILE},
    {"WHILE", Token::WHILE},

    {"AND_OP", Token::AND_OP},
    {"DEC_OP", Token::DEC_OP},
    {"DIV_ASSIGN", Token::DIV_ASSIGN},
    {"EQ_OP", Token::EQ_OP},
    {"GE_OP", Token::GE_OP},
    {"INC_OP", Token::INC_OP},
    {"LE_OP", Token::LE_OP},
    {"LEFT_ASSIGN", Token::LEFT_ASSIGN},
    {"LEFT_OP", Token::LEFT_OP},
    {"NE_OP", Token::NE_OP},
    {"OR_OP", Token::OR_OP},
    {"PTR_OP", Token::PTR_OP},
    {"MOD_ASSIGN", Token::MOD_ASSIGN},
    {"MUL_ASSIGN", Token::MUL_ASSIGN},
    {"OR_ASSIGN", Token::OR_ASSIGN},
    {"RIGHT_ASSIGN", Token::RIGHT_ASSIGN},
    {"RIGHT_OP", Token::RIGHT_OP},
    {"SUB_ASSIGN", Token::SUB_ASSIGN},
    {"XOR_ASSIGN", Token::XOR_ASSIGN},

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
    {"goto", Token::GOTO},
    {"inline", Token::INLINE},
    {"int", Token::INT},
    {"long", Token::LONG},
    {"register", Token::REGISTER},
    {"restrict", Token::RESTRICT},
    {"return", Token::RETURN},
    {"signed", Token::SIGNED},
    {"sizeof", Token::SIZEOF},
    {"static", Token::STATIC},
    {"struct", Token::STRUCT},
    {"switch", Token::SWITCH},
    {"union", Token::UNION},
    {"unsigned", Token::UNSIGNED},
    {"typedef", Token::TYPEDEF},
    {"void", Token::VOID},
    {"volatile", Token::VOLATILE},
    {"while", Token::WHILE},

    // symbols
    {R"("+=")", Token::ADD_ASSIGN},
    {"&=", Token::AND_ASSIGN},
    {"&&", Token::AND_OP},
    {"--", Token::DEC_OP},
    {R"("/=")", Token::DIV_ASSIGN},
    {R"("...")", Token::ELIPSIS},
    {"==", Token::EQ_OP},
    {">=", Token::GE_OP},
    {R"("++")", Token::INC_OP},
    {"<=", Token::LE_OP},
    {"<<=", Token::LEFT_ASSIGN},
    {"<<", Token::LEFT_OP},
    {"%=", Token::MOD_ASSIGN},
    {R"("*=")", Token::MUL_ASSIGN},
    {"!=", Token::NE_OP},
    {R"("|=")", Token::OR_ASSIGN},
    {R"("||")", Token::OR_OP},
    {"->", Token::PTR_OP},
    {">>=", Token::RIGHT_ASSIGN},
    {">>", Token::RIGHT_OP},
    {"-=", Token::SUB_ASSIGN},
    {R"("^=")", Token::XOR_ASSIGN},
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
  rules.push("{NZ}{D}*{IS}?", Token::CONSTANT);
  rules.push("{HP}{H}+{IS}?", Token::CONSTANT);
  rules.push("0{O}*{IS}?", Token::CONSTANT);

  // character
  rules.push(R"**({CP}?'([^'\\\n]|{ES})+')**", Token::CONSTANT);

  // floats
  rules.push("{D}+{E}{FS}?", Token::CONSTANT);
  rules.push("{D}*\\.{D}+{E}?{FS}?", Token::CONSTANT);

  // string
  rules.push(R"**(({SP}?["]([^\\"\n\r]|{ES})*\"{WS}*)+)**", Token::STRING_LITERAL);

  rules.push("{WS}", Token::SPACE);

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

      if (results.id != Token::SPACE)
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
  lexertl::memory_file c_bnf("grammar/c_raw");

  if (c_bnf.data() == 0)
  {
    throw "Unable to open grammar/c_raw";
  }

  std::string c_definition(c_bnf.data(), c_bnf.data() + c_bnf.size());

  std::cout << "Building grammar" << std::endl;
  auto [grammar, start] = earley::parse_grammar(c_definition);

  auto tokens = get_tokens(file);
  earley::fast::TerminalList symbols(tokens.begin(), tokens.end());
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

  auto memstart = sbrk(0);
  std::cout << "Parsing " << symbols.size() << " tokens" << std::endl;
  earley::fast::Parser parser(built, symbols);

  size_t i;
  size_t progress = symbols.size() / 100;
  std::cout << "Progress dot indicates " << progress << " tokens" << std::endl;
  try {
    Timer timer;
    for (i = 0; i != symbols.size(); ++i)
    {
      if ((i+1) % progress == 0)
      {
        std::cout << '.' << std::flush;
      }
      parser.parse(i);
    }
    std::cout << "Parsing took " << timer.count<std::chrono::microseconds>()
      << " microseconds" << std::endl;

    auto memend = sbrk(0);

    std::cout << "Used " 
      << (static_cast<char*>(memend) - static_cast<char*>(memstart)) / 1000
      << "kb of memory" << std::endl;

    parser.print_stats();
  } catch(...)
  {
    if (dump)
    {
      for (size_t j = 0; j <= i; ++j)
      {
        std::cout << "-- Set " << j << " --" << std::endl;
        std::cout << "Symbol: " << tokens[j] << std::endl;
        parser.print_set(j);
      }
    }
    auto& position = positions[i];
    std::cout << "Error at: " << position.line << ":" << position.column << std::endl;
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
