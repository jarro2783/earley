#ifndef EARLEY_GRAMMAR_HPP_INCLUDED
#define EARLEY_GRAMMAR_HPP_INCLUDED

#include "earley.hpp"
#include <memory>

namespace earley
{
  namespace ast
  {
    class Grammar;

    typedef std::shared_ptr<Grammar> GrammarPtr;
    typedef ActionResult<GrammarPtr> GrammarNode;
    typedef std::vector<GrammarNode> GrammarNodeList;

    class Grammar
    {
      public:
      virtual ~Grammar() = default;
    };

    template <typename T>
    class GrammarList : public Grammar
    {
      public:

      GrammarList() = default;

      GrammarList(const GrammarNode& start)
      {
        if (!std::holds_alternative<earley::values::Empty>(start))
        {
          m_list.push_back(std::get<typename T::value_type>(start));
        }
      }

      auto&
      list()
      {
        return m_list;
      }

      private:
      T m_list;;
    };

    class Rule : public Grammar
    {
    };

    class Nonterminal : public Grammar
    {
    };

    template <typename T>
    GrammarNode
    action_append_list(std::vector<GrammarNode>& nodes)
    {
      GrammarList<T>* list;

      std::cerr << "Trying to append to list" << std::endl;
      if (nodes.size() == 2 && std::holds_alternative<GrammarPtr>(nodes[0]) &&
          (list = dynamic_cast<GrammarList<T>*>(std::get<GrammarPtr>(nodes[0]).get())
           ) != nullptr)
      {
        std::cerr << "Appending to list of size " << list->list().size() << std::endl;
        list->list().push_back(std::get<typename T::value_type>(nodes[1]));
        return nodes[0];
      }

      return earley::values::Failed();
    }

    template <typename T>
    GrammarNode
    action_create_list(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() == 0)
      {
        std::cerr << "Creating a list of size 0" << std::endl;
        return std::make_shared<GrammarList<T>>();
      }
      else if (nodes.size() == 1)
      {
        std::cerr << "Creating a list of size 1" << std::endl;
        return std::make_shared<GrammarList<T>>(nodes[0]);
      }

      return earley::values::Failed();
    }
  }
}

#endif
