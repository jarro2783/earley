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

    class GrammarList : public Grammar
    {
      public:

      GrammarList() = default;

      GrammarList(const GrammarNode& start)
      {
        if (!std::holds_alternative<earley::values::Empty>(start)
            && !std::holds_alternative<earley::values::Failed>(start))
        {
          m_list.push_back(start);
        }
      }

      auto&
      list()
      {
        return m_list;
      }

      const auto&
      list() const
      {
        return m_list;
      }

      private:
      std::vector<GrammarNode> m_list;;
    };

    class GrammarString : public Grammar
    {
    };

    class Rule : public Grammar
    {
      public:

      Rule()
      {
        productions.push_back(Epsilon());
      }

      Rule(const GrammarNode& productions)
      {
        process_productions(productions);
      }

      Rule(const GrammarNode& productions, const GrammarNode&)
      {
        process_productions(productions);
      }

      private:

      void
      process_productions(const GrammarNode& productions)
      {
        auto& ptr = std::get<GrammarPtr>(productions);
        auto list = dynamic_cast<const GrammarList*>(ptr.get());
        // each of these should be either a name or a literal
        for (auto& production: list->list())
        {
          auto name = dynamic_cast<const GrammarString*>(ptr.get());
          if (name != nullptr)
          {
            continue;
          }
        }
      }

      std::vector<Production> productions;
    };

    class Nonterminal : public Grammar
    {
    };

    inline
    GrammarNode
    action_append_list(std::vector<GrammarNode>& nodes)
    {
      GrammarList* list = nullptr;

      std::cerr << "Trying to append to list" << std::endl;
      if (nodes.size() == 2 && std::holds_alternative<GrammarPtr>(nodes[0]) &&
          (list = dynamic_cast<GrammarList*>(std::get<GrammarPtr>(nodes[0]).get())
           ) != nullptr)
      {
        std::cerr << "Appending to list of size " << list->list().size() << std::endl;

        auto& rhs = nodes[1];
        if (!std::holds_alternative<GrammarPtr>(rhs))
        {
          list->list().push_back(rhs);
        }
        else
        {
          // it holds a grammar node, so we collapse a list if it
          // has one
          auto rhs_ptr = std::get<GrammarPtr>(rhs).get();
          auto rhs_list = dynamic_cast<const GrammarList*>(rhs_ptr);

          if (rhs_list != nullptr)
          {
            list->list().insert(list->list().end(),
              rhs_list->list().begin(), rhs_list->list().end());
          }
          else
          {
            list->list().insert(list->list().begin(), std::get<GrammarPtr>(rhs));
          }
        }

        return nodes[0];
      }
      else
      {
        std::cerr << "Unable to append to list" << std::endl;
      }

      return earley::values::Failed();
    }

    inline
    GrammarNode
    action_create_list(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() == 0)
      {
        std::cerr << "Creating a list of size 0" << std::endl;
        return std::make_shared<GrammarList>();
      }
      else if (nodes.size() == 1)
      {
        std::cerr << "Creating a list of size 1" << std::endl;
        return std::make_shared<GrammarList>(nodes[0]);
      }

      return earley::values::Failed();
    }

    inline
    GrammarNode
    action_create_string(std::vector<GrammarNode>& nodes)
    {
    }

    inline
    GrammarNode
    action_append_string(std::vector<GrammarNode>& nodes)
    {
    }

    inline
    GrammarNode
    action_rule(std::vector<GrammarNode>& nodes)
    {
      switch (nodes.size())
      {
        case 0:
        case 1:
        case 2:
        return std::make_shared<Rule>();
        break;
      }

      return values::Empty();
    }
  }
}

#endif
