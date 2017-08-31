#include "catch.hpp"
#include "earley/fast/grammar.hpp"

using namespace earley::fast::grammar;

TEST_CASE("Next nonterminal", "[names]")
{
  NonterminalIndices indices;

  CHECK(indices.index("foo") == 0);
  CHECK(indices.index("bar") == 1);
  CHECK(indices.index("foo") == 0);
  CHECK(indices.index("bar") == 1);
  CHECK(indices.index("baz") == 2);
}

TEST_CASE("Build grammar", "[grammar]")
{
  earley::Grammar grammar{
    {
      "S", {
        {{'a'}},
        {{"S", 'a'}},
      },
    },
  };

  Grammar g(grammar);

  CHECK(g.rules("S").size() == 2);
}
