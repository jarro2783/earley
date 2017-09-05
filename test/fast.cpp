#include "catch.hpp"
#include "earley/fast/grammar.hpp"
#include "earley/fast/items.hpp"

using namespace earley::fast::grammar;
using namespace earley::fast;

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

TEST_CASE("Items generation", "[items]")
{
  std::vector<RuleList> rules{
    {
      {0, {{0, false}, {'a', true}}},
      {0, {{'a', true}}},
    },
  };

  Items items(rules);

  const Rule* r00 = &rules[0][0];
  const Rule* r01 = &rules[0][1];

  auto i000 = items.get_item(r00, 0);
  auto i010 = items.get_item(r01, 0);

  REQUIRE(&i000->rule() == r00);
  REQUIRE(&i010->rule() == r01);

  CHECK(i000->position() == r00->begin());
  CHECK(i010->position() == r01->begin());

  auto i001 = items.get_item(r00, 1);
  REQUIRE(&i001->rule() == r00);
  CHECK(i001->position() == r00->begin()+1);

  CHECK_THROWS(items.get_item(r00, 10));

  CHECK(i000->end() == r00->end());
  CHECK(i000->dot() == r00->begin());
  CHECK(i000->dot() == i000->position());
}
