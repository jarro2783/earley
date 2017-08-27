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
