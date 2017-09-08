// This is just playing around with lexertl at the moment.

#include "lexertl/generator.hpp"
#include <iostream>
#include "lexertl/lookup.hpp"

enum Tokens
{
  NUMBER = 1,
  IDENTIFIER,
};

int main()
{
  lexertl::rules rules;
  lexertl::state_machine sm;

  rules.push("[0-9]+", NUMBER);
  rules.push("[a-z]+", IDENTIFIER);
  rules.push("[ \n\t]", sm.skip());

  lexertl::generator::build(rules, sm);

  std::string input("abc 012 34");
  lexertl::smatch results(input.begin(), input.end());

  // Read ahead
  lexertl::lookup(sm, results);

  while (results.id != 0)
  {
    std::cout << "Id: " << static_cast<int>(results.id) << ", Token: '" <<
        results.str () << "'\n";
    lexertl::lookup(sm, results);
  }

  return 0;
}
