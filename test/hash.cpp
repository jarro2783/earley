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

struct DestructCounter
{
  ~DestructCounter() {
    ++destructs;
  }

  static int destructs;
};

int DestructCounter::destructs = 0;

TEST_CASE("Next prime", "[prime]")
{
  using earley::detail::next_prime;

  // The algorithm doesn't work for 1, but I'm not too fussed
  CHECK(next_prime(1) == 3);
  CHECK(next_prime(2) == 5);
  CHECK(next_prime(3) == 5);
  CHECK(next_prime(5) == 7);
  CHECK(next_prime(101) == 103);
}

TEST_CASE("Insert", "[insert]")
{
  earley::HashSet<int> h;
  CHECK(h.size() == 0);
  CHECK(h.capacity() == 0);

  h.insert(150);
  REQUIRE(h.count(150) == 1);
  CHECK(h.size() == 1);
  CHECK(h.capacity() > 1);
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

TEST_CASE("Resize", "[resize]")
{
  constexpr int initial = 5;
  constexpr int inserts = 15;
  {
    earley::HashSet<std::unique_ptr<DestructCounter>> h(3);
    CHECK(h.capacity() == initial);

    for (size_t i = 0; i != inserts; ++i)
    {
      h.insert(std::make_unique<DestructCounter>());
    }

    CHECK(h.capacity() > initial);
    CHECK(DestructCounter::destructs == 0);
  }

  CHECK(DestructCounter::destructs == inserts);
}
