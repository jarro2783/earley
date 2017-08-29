#include "catch.hpp"

#include "earley.hpp"
#include "earley/grammar_util.hpp"

using earley::scan_char;
using earley::scan_range;

TEST_CASE("Simple first sets", "[firsts]")
{
  std::vector<earley::RuleList> rules{
    {
      {0, {scan_char('a')}},
    },
  };

  earley::ParseGrammar grammar(0, rules);

  auto firsts = earley::first_sets(grammar);

  REQUIRE(firsts.count(0));
  auto& first0 = firsts[0];
  CHECK(first0.size() == 1);
  CHECK(first0.count('a'));
}

TEST_CASE("Epsilon in first set", "[firsts]")
{
  std::vector<earley::RuleList> rules{
    {
      {0, {}},
      {0, {scan_char('a')}},
    },
  };

  earley::ParseGrammar grammar(0, rules);

  auto firsts = earley::first_sets(grammar);

  REQUIRE(firsts.count(0));

  auto& first0 = firsts[0];
  CHECK(first0.size() == 2);
  CHECK(first0.count(-1));
  CHECK(first0.count('a'));
}

TEST_CASE("Complex first set", "[firsts]")
{
  std::vector<earley::RuleList> rules{
    {
      {0, {}},
      {0, {scan_char('a')}},
    },
    {
      {1, {scan_range('b', 'd')}},
    },
    {
      {2, {0, 1}},
    },
  };

  earley::ParseGrammar grammar(0, rules);

  auto firsts = earley::first_sets(grammar);

  REQUIRE(firsts.count(0));
  REQUIRE(firsts.count(1));
  REQUIRE(firsts.count(2));

  auto& first0 = firsts[0];
  auto& first1 = firsts[1];
  auto& first2 = firsts[2];

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
}

SCENARIO("First set of symbol sequence", "[firsts]")
{
  GIVEN("An empty sequence")
  {
    std::vector<earley::Entry> sequence;
    auto first = earley::first_set(sequence.begin(), sequence.end(), {});
    CHECK(first.size() == 1);
    CHECK(first.count(earley::EPSILON));
  }

  GIVEN("A sequence beginning with a terminal") {
    std::vector<earley::Entry> terminal{scan_char('a')};
    auto first = earley::first_set(terminal.begin(), terminal.end(), {});

    CHECK(first.size() == 1);
    CHECK(first.count('a'));
  }

  GIVEN("A sequence with a terminating nonterminal") {
    std::vector<earley::Entry> nonterminal{0};
    earley::FirstSet firsts{
      {0, {'a'}},
    };

    auto first = earley::first_set(nonterminal.begin(), nonterminal.end(), firsts);

    CHECK(first.size() == 1);
    CHECK(first.count('a'));
  }

  GIVEN("A sequence with an epsilon nonterminal") {
    std::vector<earley::Entry> sequence{0, 1};
    earley::FirstSet firsts{
      {0, {earley::EPSILON, 'a'}},
      {1, {'b'}},
    };

    auto first = earley::first_set(sequence.begin(), sequence.end(), firsts);

    CHECK(first.size() == 3);
    CHECK(first.count(earley::EPSILON));
    CHECK(first.count('a'));
    CHECK(first.count('b'));
  }
}

SCENARIO("Follow set", "[follow]")
{
  GIVEN("A basic grammar")
  {
    std::vector<earley::RuleList> rules
    {
      {
        {0, {1, scan_char('a')}},
      },
      {
        {1, {'b'}},
      },
    };

    earley::ParseGrammar grammar(0, rules);

    auto firsts = earley::first_sets(grammar);
    auto follow = earley::follow_sets(grammar, firsts);

    REQUIRE(follow.count(0));
    REQUIRE(follow.count(1));
    CHECK(follow[0].size() == 1);
    CHECK(follow[0].count(earley::END_OF_INPUT));
    CHECK(follow[1].size() == 1);
    CHECK(follow[1].count('a'));
  }
}

SCENARIO("Insert scanner", "[insert]")
{
  std::unordered_set<int> set;

  WHEN("The set is empty")
  {
    THEN("insert_scanner returns true")
    {
      CHECK(earley::insert_scanner(earley::scan_range('a', 'c'), set));
      CHECK(set.count('a'));
      CHECK(set.count('b'));
      CHECK(set.count('c'));
    }
  }
}
