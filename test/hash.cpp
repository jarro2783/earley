#include "catch.hpp"
#include "earley_hash_set.hpp"

TEST_CASE("Insert", "insert one item")
{
  earley::HashSet<int> h;
  h.insert(150);
  REQUIRE(h.count(150) == 1);
}
