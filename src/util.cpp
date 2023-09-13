#include "earley/util.hpp"
#include <sstream>

namespace earley
{

std::string escape_character(char c)
{
  if (c < ' ')
  {
    std::ostringstream os;
    os << '\\' << std::hex << static_cast<int>(c);
    return os.str();
  }
  else
  {
    return std::string(1, c);
  }
}

}
