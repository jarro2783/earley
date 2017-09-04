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

TEST_CASE("Rules", "[rule]")
{
  Rule rule(0,
    {
      {0, false},
      {1, false},
      {2, false},
    }
  );

  CHECK(rule.nonterminal() == 0);
  CHECK(rule.begin() != rule.end());

  for (int i = 0; i != 3; ++i)
  {
    REQUIRE(rule.begin() + i < rule.end());
  }

  CHECK(rule.begin() + 3 == rule.end());
}

TEST_CASE("Invert rules", "[invert]")
{
  std::vector<Symbol> symbols{
    {0, false},
    {32, true},
    {1, false},
  };
  Rule rule0(0, symbols);
  std::vector<std::vector<const Rule*>> inverted;

  invert_rule(inverted, &rule0, *rule0.begin());

  REQUIRE(inverted.size() == 1);
  CHECK(inverted[0].size() == 1);
  CHECK(inverted[0][0] == &rule0);

  invert_rule(inverted, &rule0, *(rule0.begin()+1));

  REQUIRE(inverted.size() == 1);
  REQUIRE(inverted[0].size() == 1);

  invert_rule(inverted, &rule0, *(rule0.begin()+2));

  REQUIRE(inverted.size() == 2);
  REQUIRE(inverted[1].size() == 1);
  CHECK(inverted[1][0] == &rule0);

  Rule rule1(1, {
    {1, false},
    {0, false},
  });

  invert_rule(inverted, &rule1, *rule1.begin());

  REQUIRE(inverted.size() == 2);
  REQUIRE(inverted[1].size() == 2);
  CHECK(inverted[1][1] == &rule1);

  invert_rule(inverted, &rule1, *(rule1.begin() + 1));

  REQUIRE(inverted.size() == 2);
  REQUIRE(inverted[0].size() == 2);
  CHECK(inverted[0][1] == &rule1);
}

TEST_CASE("Nullable rules", "[nullable]")
{
  // 0 is nullable
  // 1 refers to 0
  // 2 is not nullable
  std::vector<RuleList> rules{
    {
      {0, {}},
      {0, {{2, false}}},
    },
    {
      {1, {{0, false}}},
      {1, {{1, false}, {2, false}}},
    },
    {
      {2, {{32, true}, {45, true}}},
    },
    {
      {3, {{0, false}, {1, false}, {2, false}}},
    },
  };

  auto nullable = find_nullable(rules);

  REQUIRE(nullable.size() == 4);
  CHECK(nullable[0]);
  CHECK(nullable[1]);
  CHECK(!nullable[2]);
  CHECK(!nullable[3]);
}

TEST_CASE("Build grammar", "[grammar]")
{
  earley::Grammar grammar{
    {
      "S", {
        {{}},
        {{'a'}},
        {{"S", 'a'}},
      },
    },
  };

  Grammar g("S", grammar);

  CHECK(g.rules("S").size() == 3);
  CHECK(&g.rules("S") == &g.rules(0));
  REQUIRE(g.start() == 1);
  CHECK(g.nullable(0));
  CHECK(g.nullable(1));
}
