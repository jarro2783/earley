#include <chrono>

#include <earley/fast.hpp>
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
    CONST,
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    EXTERN,
    FOR,
    TYPEDEF,
    VOID,
    WHILE,

    // Symbols
    RIGHT_OP,

    // Others
    IDENTIFIER,
    I_CONSTANT,
  };
}

std::vector<int>
get_tokens(const char* file)
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  std::initializer_list<std::pair<const char*, Token>> symbols = {
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
    {"for", Token::FOR},
    {"typedef", Token::TYPEDEF},
    {"void", Token::VOID},
  };

  std::vector<int> characters{
    '=',
    ':',
    ';',
    ',',
    '!',
    '&',
    '~',
    '.',
    '>',
    '<',
  };

  for (auto& [text, token] : symbols) {
    rules.push(text, static_cast<int>(token));
  }

  for (auto c : characters)
  {
    rules.push(std::string(1, c), c);
  }

  rules.insert_macro("WS", "[ \\t\\v\\n\\f]");
  rules.insert_macro("L", "[A-Za-z_]");
  rules.insert_macro("A", "[A-Za-z_0-9]");
  rules.insert_macro("NZ", "[1-9]");
  rules.insert_macro("D", "[0-9]");
  rules.insert_macro("HP", "0[xX]");
  rules.insert_macro("H", "[a-fA-F0-9]");

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

  rules.push(">>", Token::RIGHT_OP);

  rules.push("{L}{A}*", Token::IDENTIFIER);
  rules.push("{NZ}{D}*", Token::I_CONSTANT);
  rules.push("{HP}{H}+", Token::I_CONSTANT);

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
      throw std::string("Unrecognised token starting with ") +
        std::string(results.first, std::min(results.first + 10, mf.data() + mf.size()));
    }
    else
    {
      tokens.push_back(results.id);
    }

    lexertl::lookup(sm, results);
  }

  return tokens;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Please provide a C file" << std::endl;
  }

  try
  {
    auto tokens = get_tokens(argv[1]);
  }
  catch (const char* c)
  {
    std::cout << "Error parsing: " << c << std::endl;
    return 1;
  }
  catch (const std::string& s)
  {
    std::cout << "Error parsing: " << s << std::endl;
    return 1;
  }

  return 0;
}
