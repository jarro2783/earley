#include "catch.hpp"
#include "earley/fast.hpp"
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

  auto firsts = first_sets(rules);
  auto follows = follow_sets(0, rules, firsts);
  std::vector<bool> nullable{false};

  Items items(rules, firsts, follows, nullable);

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

SCENARIO("First set of symbol sequence", "[firsts]")
{
  GIVEN("An empty sequence")
  {
    std::vector<Symbol> sequence;
    auto first = first_set(sequence.begin(), sequence.end(), {});
    CHECK(first.size() == 1);
    CHECK(first.count(EPSILON));
  }

  GIVEN("A sequence beginning with a terminal") {
    std::vector<Symbol> terminal{{'a', true}};
    auto first = first_set(terminal.begin(), terminal.end(), {});

    CHECK(first.size() == 1);
    CHECK(first.count('a'));
  }

  GIVEN("A sequence with a terminating nonterminal") {
    std::vector<Symbol> nonterminal{{0, false}};
    FirstSets firsts{
      {0, {'a'}},
    };

    auto first = first_set(nonterminal.begin(), nonterminal.end(), firsts);

    CHECK(first.size() == 1);
    CHECK(first.count('a'));
  }

  GIVEN("A sequence with an epsilon nonterminal") {
    std::vector<Symbol> sequence{{0, false}, {1, false}};
    FirstSets firsts{
      {0, {EPSILON, 'a'}},
      {1, {'b'}},
    };

    auto first = first_set(sequence.begin(), sequence.end(), firsts);

    CHECK(first.size() == 2);
    CHECK(first.count('a'));
    CHECK(first.count('b'));
  }

  GIVEN("A sequence that ends in an epsilon") {
    std::vector<Symbol> sequence{{0, false}, {1, false}};
    FirstSets firsts{
      {0, {EPSILON, 'a'}},
      {1, {EPSILON, 'b'}},
    };

    auto first = first_set(sequence.begin(), sequence.end(), firsts);

    CHECK(first.size() == 3);
    CHECK(first.count(EPSILON));
    CHECK(first.count('a'));
    CHECK(first.count('b'));
  }
}

TEST_CASE("Simple first sets", "[firsts]")
{
  std::vector<RuleList> rules{
    {
      {0, {{'a', true}}},
    },
  };

  auto firsts = first_sets(rules);

  REQUIRE(firsts.count(0));
  auto& first0 = firsts[0];
  CHECK(first0.size() == 1);
  CHECK(first0.count('a'));
}

TEST_CASE("Epsilon in first set", "[firsts]")
{
  std::vector<RuleList> rules{
    {
      {0, {}},
      {0, {{'a', true}}},
    },
  };

  auto firsts = first_sets(rules);

  REQUIRE(firsts.count(0));

  auto& first0 = firsts[0];
  CHECK(first0.size() == 2);
  CHECK(first0.count(-1));
  CHECK(first0.count('a'));
}

TEST_CASE("Complex first set", "[firsts]")
{
  std::vector<RuleList> rules{
    {
      {0, {}},
      {0, {{'a', true}}},
    },
    {
      {1, {{'b', true}}},
      {1, {{'c', true}}},
      {1, {{'d', true}}},
    },
    {
      {2, {{0, false}, {1, false}}},
    },
    {
      {3, {{4, false}, {'e', true}}},
    },
    {
      {4, {}},
    },
  };

  auto firsts = first_sets(rules);

  for (size_t i = 0; i != 5; ++i)
  {
    REQUIRE(firsts.count(i));
  }

  auto& first0 = firsts[0];
  auto& first1 = firsts[1];
  auto& first2 = firsts[2];
  auto& first3 = firsts[3];
  auto& first4 = firsts[4];

  CHECK(first0.count(-1));
  CHECK(first0.count('a'));

  CHECK(first1.size() == 3);
  CHECK(first1.count('b'));
  CHECK(first1.count('c'));
  CHECK(first1.count('d'));

  CHECK(first2.size() == 4);
  CHECK(first2.count('a'));
  CHECK(first2.count('b'));
  CHECK(first2.count('c'));
  CHECK(first2.count('d'));

  CHECK(first3.size() == 1);
  CHECK(first3.count('e'));

  CHECK(first4.size() == 1);
  CHECK(first4.count(-1));
}

SCENARIO("Follow set", "[follow]")
{
  GIVEN("A basic grammar")
  {
    std::vector<RuleList> rules
    {
      {
        {0, {{1, false}, {'a', true}}},
      },
      {
        {1, {{'b', true}}},
      },
    };

    auto firsts = first_sets(rules);
    auto follow = follow_sets(0, rules, firsts);

    REQUIRE(follow.count(0));
    REQUIRE(follow.count(1));
    CHECK(follow[0].size() == 1);
    CHECK(follow[0].count(earley::END_OF_INPUT));
    CHECK(follow[1].size() == 1);
    CHECK(follow[1].count('a'));
  }
}

TEST_CASE("Sequence lookahead", "[lookahead]")
{
  std::vector<RuleList> rules
  {
    {
      {0, {{1, false}}},
    },
    {
      {1, {{'a', true}}},
      {1, {{'b', true}}},
    },
    {
      {2, {}},
    },
    {
      {3, {{2, false}, {';', true}}},
    },
    {
      {4, {{3, false}, {1, false}}},
    },
  };

  auto firsts = first_sets(rules);
  auto follows = follow_sets(0, rules, firsts);

  CHECK(firsts[3].count(';') != 0);
  CHECK(firsts[3].size() == 1);

  Items items(rules, firsts, follows, {false, false});

  auto& r00 = rules[0][0];

  auto lookahead = sequence_lookahead(r00, r00.begin(), firsts, follows);

  CHECK(lookahead.size() == 2);
  CHECK(lookahead.count('a') != 0);
  CHECK(lookahead.count('b') != 0);

  auto& r30 = rules[3][0];
  auto lookahead2 = sequence_lookahead(r30, r30.begin(), firsts, follows);
  CHECK(lookahead2.size() == 1);
  CHECK(lookahead2.count(';') != 0);
}

TEST_CASE("ItemSet distances", "[itemset]")
{
  using earley::fast::ItemSetCore;
  using earley::fast::ItemSet;

  std::vector<RuleList> rules
  {
    {
      {0, {{}}},
      {0, {{1, false}}},
    },
    {
      {1, {{'a', true}, {'b', true}}},
    },
  };

  Rule& r1 = rules[1][0];

  Item first(&r1, r1.begin(), earley::HashSet<int>());
  Item second(&r1, r1.begin()+1, earley::HashSet<int>());

  ItemSetCore core;
  ItemSet set(&core);

  set.add_start_item(&first, 2);
  set.add_derived_item(&second, 0);

  CHECK(set.actual_distance(0) == 2);
  CHECK(set.actual_distance(1) == 2);
}
