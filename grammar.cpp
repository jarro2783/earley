#include <typeinfo>

#include "grammar.hpp"

namespace earley
{

namespace ast
{

class InvalidGrammar
{
};

namespace
{

template <typename T>
void
check(GrammarNode node)
{
  if (!holds<T>(node))
  {
    std::cerr << "Holds " << node.index() << std::endl;
    throw InvalidGrammar();
  }
}

template <typename T, typename U>
T
checked_cast(U* u)
{
  T t = dynamic_cast<T>(u);
  if (t == nullptr)
  {
    std::cerr << "Type is: " << typeid(*u).name() << std::endl;
    throw InvalidGrammar();
  }

  return t;
}

void
print_rule(GrammarPtr ptr)
{
  auto rule = checked_cast<const Rule*>(ptr.get());
  auto& productions = rule->productions();

  for (auto& p: productions)
  {
    if (holds<std::string>(p))
    {
      std::cout << " " << get<std::string>(p);
    }
  }
}

void
print_nonterminal(GrammarNode nt)
{
  check<GrammarPtr>(nt);
  auto ptr = get<GrammarPtr>(nt);
  auto node = checked_cast<const earley::ast::GrammarNonterminal*>(ptr.get());

  std::cout << node->name() << " ->";

  auto& rules = node->rules();
  auto iter = rules.begin();

  if (iter != rules.end())
  {
    print_rule(*iter);
    ++iter;
  }

  while (iter != rules.end())
  {
    std::cout << std::endl << "  |";
    print_rule(*iter);
    ++iter;
  }
  std::cout << std::endl;
}

}

void
print_grammar(GrammarNode grammar)
{
  check<GrammarPtr>(grammar);

  auto ptr = get<GrammarPtr>(grammar);
  auto list = checked_cast<const GrammarList*>(ptr.get());

  for (auto& nt : list->list())
  {
    print_nonterminal(nt);
  }
}

}

}
