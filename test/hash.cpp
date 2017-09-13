#include "catch.hpp"
#include "earley_hash_set.hpp"

template <typename Op>
struct CountedOp
{
  CountedOp() = default;

  template <typename... T>
  auto
  operator()(T&&... t)
  {
    ++m_counter;
    return Op()(t...);
  }

  static
  int
  counter()
  {
    return m_counter;
  }

  private:
  static int m_counter;
};

template <typename T>
int CountedOp<T>::m_counter = 0;

TEST_CASE("Insert", "[insert]")
{
  earley::HashSet<int> h;
  h.insert(150);
  REQUIRE(h.count(150) == 1);
}

TEST_CASE("Custom hash and equals", "[hash]")
{
  typedef CountedOp<std::hash<int>> Hash;
  typedef CountedOp<std::equal_to<int>> Equal;
  earley::HashSet<int, Hash, Equal> h;

  CHECK(Hash::counter() == 0);
  CHECK(Equal::counter() == 0);

  h.insert(5);

  CHECK(Hash::counter() == 1);

  // this is a tad fragile because it depends on the number of collisions
  CHECK(Equal::counter() == 0);

  h.find(5);
  CHECK(Hash::counter() == 2);
  CHECK(Equal::counter() == 1);
}
