#ifndef EARLEY_GRAMMAR_HPP_INCLUDED
#define EARLEY_GRAMMAR_HPP_INCLUDED

#include <memory>
#include <ostream>

#include "earley.hpp"
#include "earley/variant.hpp"

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
      public:

      GrammarString() = default;

      GrammarString(char c)
      : m_string(1, c)
      {
        std::cout << "Create string " << m_string << std::endl;
      }

      const std::string&
      string() const
      {
        return m_string;
      }

      std::string&
      string()
      {
        return m_string;
      }

      private:
      std::string m_string;
    };

    class GrammarRange : public Grammar
    {
      public:
      GrammarRange(char begin, char end)
      : m_begin(begin)
      , m_end(end)
      {
      }

      auto
      make_scanner() const
      {
        return scan_range(m_begin, m_end);
      }

      friend
      std::ostream&
      operator<<(std::ostream&, const GrammarRange&);

      private:
      char m_begin;
      char m_end;
    };

    class GrammarNonterminal : public Grammar
    {
      public:
      GrammarNonterminal(std::string name,
        std::vector<GrammarPtr> rules)
      : m_name(std::move(name))
      , m_rules(std::move(rules))
      {
      }

      const std::string&
      name() const
      {
        return m_name;
      }

      auto&
      rules() const
      {
        return m_rules;
      }

      private:
      std::string m_name;
      std::vector<GrammarPtr> m_rules;
    };

    class Rule : public Grammar
    {
      public:

      Rule()
      {
        std::cout << "Empty rule" << std::endl;
      }

      Rule(const GrammarNode& productions)
      {
        process_productions(productions);
      }

      Rule(const GrammarNode& productions, const GrammarNode&)
      {
        process_productions(productions);
      }

      auto&
      productions() const
      {
        return m_productions;
      }

      private:

      void
      process_productions(const GrammarNode& productions)
      {
        auto& ptr = std::get<GrammarPtr>(productions);
        auto list = dynamic_cast<const GrammarList*>(ptr.get());
        // each of these should be either a name or a literal
        std::cerr << "Parsed rule: ";
        for (auto& production: list->list())
        {
          if (std::holds_alternative<GrammarPtr>(production))
          {
            auto ptr = std::get<GrammarPtr>(production).get();
            const GrammarString* name = nullptr;
            const GrammarRange* range = nullptr;

            if ((name = dynamic_cast<const GrammarString*>(ptr)) != nullptr)
            {
              std::cerr << name->string() << ' ';
              m_productions.push_back(name->string());
            }
            else if ((range = dynamic_cast<const GrammarRange*>(ptr)) != nullptr)
            {
              // Build a range here out of a GrammarRange
              std::cerr << "Range: " << *range << std::endl;
              m_productions.push_back(range->make_scanner());
            }
          }
          else if (holds<char>(production))
          {
            std::cerr << get<char>(production);
            m_productions.push_back(get<char>(production));
          }
        }
        std::cerr << std::endl;
      }

      std::vector<Production> m_productions;
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
            list->list().push_back(std::get<GrammarPtr>(rhs));
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
      std::cout << "Create string" << std::endl;
      switch (nodes.size())
      {
        case 0:
        return std::make_shared<GrammarString>();

        case 1:
        if (!std::holds_alternative<char>(nodes[0]))
        {
          return values::Failed();
        }
        return std::make_shared<GrammarString>(std::get<char>(nodes[0]));

        default:
        return values::Failed();
      }
    }

    inline
    GrammarNode
    action_append_string(std::vector<GrammarNode>& nodes)
    {
      std::cout << "Append string" << std::endl;
      if (nodes.size() != 2 || !std::holds_alternative<GrammarPtr>(nodes[0]))
      {
        return values::Failed();
      }

      auto ptr = std::get<GrammarPtr>(nodes[0]);
      auto gstring = dynamic_cast<GrammarString*>(ptr.get());

      if (gstring == nullptr)
      {
        return values::Failed();
      }

      auto& string = gstring->string();

      if (std::holds_alternative<char>(nodes[1]))
      {
        string.append(1, std::get<char>(nodes[1]));
        std::cout << "Made string " << string << std::endl;
      }
      else if (std::holds_alternative<GrammarPtr>(nodes[1]))
      {
        auto rhs = dynamic_cast<GrammarString*>(std::get<GrammarPtr>(nodes[1]).get());
        if (rhs == nullptr)
        {
          std::cout << "String append failed" << std::endl;
          return values::Failed();
        }

        string.append(rhs->string());
        std::cout << "Made string " << string << std::endl;
      }
      else
      {
        std::cout << "String append failed" << std::endl;
        return values::Failed();
      }

      return ptr;
    }

    inline
    GrammarNode
    action_rule(std::vector<GrammarNode>& nodes)
    {
      std::cout << "Building a rule" << std::endl;
      switch (nodes.size())
      {
        case 0:
        return std::make_shared<Rule>();

        case 1:
        return std::make_shared<Rule>(nodes[0]);

        case 2:
        return std::make_shared<Rule>(nodes[0]);
        break;
      }

      return values::Empty();
    }

    inline
    GrammarNode
    action_create_range(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() != 2)
      {
        return values::Failed();
      }

      return std::make_shared<GrammarRange>(
        std::get<char>(nodes[0]),
        std::get<char>(nodes[1]));
    }

    inline
    GrammarNode
    action_create_nonterminal(std::vector<GrammarNode>& nodes)
    {
      std::cout << "Trying to make a nonterminal" << std::endl;
      if (nodes.size() != 2)
      {
        return values::Failed();
      }

      const GrammarString* name = nullptr;
      const GrammarList* rules = nullptr;

      if (holds<GrammarPtr>(nodes[0]))
      {
        name = dynamic_cast<const GrammarString*>(get<GrammarPtr>(nodes[0]).get());
      }

      if (holds<GrammarPtr>(nodes[1]))
      {
        rules = dynamic_cast<const GrammarList*>(get<GrammarPtr>(nodes[1]).get());
      }

      if (name == nullptr || rules == nullptr)
      {
        return values::Failed();
      }

      std::vector<GrammarPtr> rules_ptrs;
      for (auto& node: rules->list())
      {
        if (!holds<GrammarPtr>(node))
        {
          return values::Failed();
        }

        rules_ptrs.push_back(get<GrammarPtr>(node));
      }

      std::cout << "Built nonterminal" << std::endl;

      return std::make_shared<GrammarNonterminal>(name->string(), rules_ptrs);
    }
  }

  void
  parse_ebnf(const std::string& input, bool debug, bool timing,
    const std::string& text = std::string());
}

#endif
