#ifndef __PARSER_HPP_1457164471__
#define __PARSER_HPP_1457164471__

#include <memory>
#include <string>

#include "tokenizer.hpp"
#include "object.hpp"

namespace detail {
bool & parser_colog_switch();
} // namespace detail

namespace varlisp {

typedef std::string::const_iterator StrIterator;

// class Environment;
class Parser {
public:
    Parser() = default;

    explicit Parser(const std::string& scripts) { m_toknizer.append(scripts); }
    virtual ~Parser() = default;

public:
    Parser(Parser&&) = default;
    Parser& operator=(Parser&&) = default;

public:
    Parser(const Parser&) = default;
    Parser& operator=(const Parser&) = default;

public:
    typedef std::tuple<int, int, int> parenthese_stack_t;
    parenthese_stack_t get_parenthese_stack() const
    {
        return m_parenthese_stack;
    }
    int parse(varlisp::Environment& env, const std::string& scripts,
              bool is_silent = false);

    int retrieve_symbols(std::vector<std::string>& symbols,
                         const char* prefix) const;

    /**
     * @brief verify '( ') balance
     *
     * @return
     */
    bool balance_preread();

protected:
    // 空白和括弧，是不纳入结构的！
    // 所谓的Token，不包括空白和括弧；
    // 它包括：
    //   Number
    //   Boolean
    //   List
    //   以及Symbol
    // 这就是说，包括操作符，也是Symbol!
    Object parseExpression();
    Object parseList();
    Object parseEnvironment();
    Object parseQuote();

    Object parseSpecialIf();
    Object parseSpecialCond();
    Object parseSpecialAnd();
    Object parseSpecialOr();
    Object parseSpecialDefine();
    Object parseSpecialLambda();

private:
    varlisp::Tokenizer m_toknizer;
    parenthese_stack_t m_parenthese_stack;
};
}  // namespace varlisp

#endif /* __PARSER_HPP_1457164471__ */
