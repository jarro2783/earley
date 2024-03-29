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

    class InvalidGrammar
    {
    };

    class Grammar
    {
      public:
      virtual ~Grammar() = default;
    };

    using GrammarPtr = std::shared_ptr<Grammar>;
    using GrammarNode = ActionResult<GrammarPtr>;
    using GrammarNodeList = std::vector<GrammarNode>;

    template <typename T>
    T& grammar_get(GrammarNode& node)
    {
      auto ptr = get<GrammarPtr>(node);
      auto& t = dynamic_cast<T&>(*ptr);

      return t;
    }

    class GrammarDescription : public Grammar
    {
      public:
      GrammarDescription(GrammarNode terminals, GrammarNode nonterminals)
      : m_t(terminals)
      , m_nt(nonterminals)
      {
      }

      GrammarNode
      terminals() const
      {
        return m_t;
      }

      GrammarNode
      nonterminals() const
      {
        return m_nt;
      }

      private:
      GrammarNode m_t;
      GrammarNode m_nt;
    };

    class GrammarList : public Grammar
    {
      public:

      GrammarList() = default;

      GrammarList(const GrammarNode& start)
      {
        if (!holds<earley::values::Empty>(start)
            && !holds<earley::values::Failed>(start))
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

    class GrammarTerminals : public Grammar
    {
      public:
      GrammarTerminals(std::vector<std::pair<std::string, int>> names)
      : m_names(std::move(names))
      {
      }

      auto&
      names() const
      {
        return m_names;
      }

      private:
      std::vector<std::pair<std::string, int>> m_names;
    };

    class Rule : public Grammar
    {
      public:

      Rule()
      {
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
        auto& ptr = get<GrammarPtr>(productions);
        auto list = dynamic_cast<const GrammarList*>(ptr.get());
        // each of these should be either a name or a literal
        for (auto& production: list->list())
        {
          if (holds<GrammarPtr>(production))
          {
            auto ptr = get<GrammarPtr>(production).get();
            const GrammarString* name = nullptr;
            const GrammarRange* range = nullptr;

            if ((name = dynamic_cast<const GrammarString*>(ptr)) != nullptr)
            {
              m_productions.push_back(name->string());
            }
            else if ((range = dynamic_cast<const GrammarRange*>(ptr)) != nullptr)
            {
              // Build a range here out of a GrammarRange
              m_productions.push_back(range->make_scanner());
            }
          }
          else if (holds<char>(production))
          {
            m_productions.push_back(get<char>(production));
          }
        }
      }

      std::vector<Production> m_productions;
    };

    class GrammarAction : public Grammar
    {
      private:
      std::string m_action_name;
      std::vector<int> m_positions;
    };

    class GrammarNumber : public Grammar
    {
      public:
      GrammarNumber(int value) : m_value(value) {}
      int value() const { return m_value; }

      void append_value(int v)
      {
        m_value *= 10 + v;
      }

      private:
      int m_value = 0;
    };

    inline
    GrammarNode
    action_construct_grammar(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() != 2)
      {
        return values::Failed();
      }

      return std::make_shared<GrammarDescription>(nodes[0], nodes[1]);
    }

    inline
    GrammarNode
    action_append_list(std::vector<GrammarNode>& nodes)
    {
      GrammarList* list = nullptr;

      if (nodes.size() == 2 && holds<GrammarPtr>(nodes[0]) &&
          (list = dynamic_cast<GrammarList*>(get<GrammarPtr>(nodes[0]).get())
           ) != nullptr)
      {
        auto& rhs = nodes[1];
        if (!holds<GrammarPtr>(rhs))
        {
          list->list().push_back(rhs);
        }
        else
        {
          // it holds a grammar node, so we collapse a list if it
          // has one
          auto rhs_ptr = get<GrammarPtr>(rhs).get();
          auto rhs_list = dynamic_cast<const GrammarList*>(rhs_ptr);

          if (rhs_list != nullptr)
          {
            list->list().insert(list->list().end(),
              rhs_list->list().begin(), rhs_list->list().end());
          }
          else
          {
            list->list().push_back(get<GrammarPtr>(rhs));
          }
        }

        return nodes[0];
      }

      return earley::values::Failed();
    }

    inline
    GrammarNode
    action_create_list(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() == 0)
      {
        return std::make_shared<GrammarList>();
      }
      else if (nodes.size() == 1)
      {
        return std::make_shared<GrammarList>(nodes[0]);
      }

      return earley::values::Failed();
    }

    inline
    GrammarNode
    action_create_string(std::vector<GrammarNode>& nodes)
    {
      switch (nodes.size())
      {
        case 0:
        return std::make_shared<GrammarString>();

        case 1:
        if (!holds<char>(nodes[0]))
        {
          return values::Failed();
        }
        return std::make_shared<GrammarString>(get<char>(nodes[0]));

        default:
        return values::Failed();
      }
    }

    inline
    GrammarNode
    action_append_string(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() != 2 || !holds<GrammarPtr>(nodes[0]))
      {
        return values::Failed();
      }

      auto ptr = get<GrammarPtr>(nodes[0]);
      auto gstring = dynamic_cast<GrammarString*>(ptr.get());

      if (gstring == nullptr)
      {
        return values::Failed();
      }

      auto& string = gstring->string();

      if (holds<char>(nodes[1]))
      {
        string.append(1, get<char>(nodes[1]));
      }
      else if (holds<GrammarPtr>(nodes[1]))
      {
        auto rhs = dynamic_cast<GrammarString*>(get<GrammarPtr>(nodes[1]).get());
        if (rhs == nullptr)
        {
          return values::Failed();
        }

        string.append(rhs->string());
      }
      else
      {
        return values::Failed();
      }

      return ptr;
    }

    inline
    GrammarNode
    action_rule(std::vector<GrammarNode>& nodes)
    {
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
        get<char>(nodes[0]),
        get<char>(nodes[1]));
    }

    inline
    GrammarNode
    action_create_nonterminal(std::vector<GrammarNode>& nodes)
    {
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

      return std::make_shared<GrammarNonterminal>(name->string(), rules_ptrs);
    }

    inline
    GrammarNode
    action_construct_terminals(std::vector<GrammarNode>& nodes)
    {
      if (nodes.size() != 1)
      {
        return values::Failed();
      }

      auto& node = nodes[0];
      auto node_ptr = get<GrammarPtr>(node).get();
      auto node_list = dynamic_cast<const GrammarList*>(node_ptr);

      if (node_list == nullptr)
      {
        return values::Failed();
      }

      std::vector<std::pair<std::string, int>> names;

      int id = 256;

      for (auto& item: node_list->list())
      {
        auto ptr = get<GrammarPtr>(item);
        auto string = dynamic_cast<const GrammarString*>(ptr.get());

        if (string == nullptr)
        {
          return values::Failed();
        }

        names.push_back({string->string(), id});
        ++id;
      }

      return std::make_shared<GrammarTerminals>(std::move(names));
    }

    inline
    GrammarNode
    action_create_number(std::vector<GrammarNode>& nodes) {
      if (nodes.size() != 1)
      {
        return values::Failed();
      }

      auto& node = nodes[0];
      auto value = get<char>(node);

      std::cout << int(value - '0') << std::endl;
      return std::make_shared<GrammarNumber>(value - '0');
    }

    inline
    GrammarNode
    action_append_number(std::vector<GrammarNode>& nodes) {
      if (nodes.size() != 2)
      {
        return values::Failed();
      }

      auto& lhs = nodes[0];
      auto& rhs = nodes[1];

      auto& num = grammar_get<GrammarNumber>(lhs);
      auto value = get<char>(rhs);

      num.append_value(value);

      std::cout << num.value();

      return lhs;
    }

  }

  void
  parse_ebnf(const std::string& input, bool debug, bool timing, bool slow,
    const std::string& text = std::string());

  std::tuple<earley::Grammar, earley::TerminalMap, std::string>
  parse_grammar(const std::string& text, bool timing, bool debug=false);

}

#endif
