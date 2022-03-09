#include "parser.hpp"

#include <sstream>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/log.hpp>
#include <sss/util/Memory.hpp>
#include <sss/util/PostionThrow.hpp>

#include <ss1x/parser/exception.hpp>
#include <ss1x/parser/oparser.hpp>

#include "builtin_helper.hpp"
#include "detail/list_iterator.hpp"
#include "environment.hpp"
#include "keyword_t.hpp"
#include "print_visitor.hpp"
#include "tokenizer.hpp"

namespace detail {
bool & parser_colog_switch()
{
    static bool is_open = false;
    return is_open;
}
} // namespace detail

#define COLOG_TRIGER_INFO(...) \
    do {\
        if (::detail::parser_colog_switch()) { \
            COLOG_INFO(__VA_ARGS__); \
        } \
    } while(false);

#define COLOG_TRIGER_ERROR(...) \
    do {\
        if (::detail::parser_colog_switch()) { \
            COLOG_ERROR(__VA_ARGS__); \
        } \
    } while(false);

#define COLOG_TRIGER_DEBUG(...) \
    do {\
        if (::detail::parser_colog_switch()) { \
            COLOG_DEBUG(__VA_ARGS__); \
        } \
    } while(false);


namespace varlisp {

class Tokenizer_stat_wrapper {
    Tokenizer& m_tz;
    bool m_active;
public:
    Tokenizer_stat_wrapper(Tokenizer& tz, bool is_active)
        : m_tz(tz), m_active(is_active)
    {
        if (m_active) {
            m_tz.push();
        }
    }

    ~Tokenizer_stat_wrapper()
    {
        if (m_active) {
            m_tz.pop();
        }
    }
};

// NOTE 如何解析 {} ？
// 如何保存？
// 创建一个Environment 然后move过去。
// {} 一个空的Environment；
// { (sym (expr))... }
// 如何取值？
// 双::!
// (format 1 "{}" env-name::sym-name ...)
// (with env-name
//    (expr) ...)
// 后者是借鉴js中的with语法；with块内的操作，默认是对该标识符
// 作用域的属性、方法进行操作。可以少写几个字。
// 我这里，是想操作其中的数据；
// 不过，::的实现，貌似有些麻烦——我需要修改标识符的解析了。
// 在这之前，可以额外定义函数。比如
// (using env-name sym-name) 来获取特定的值，以及函数；using，也可以继续嵌套……

// scripts
//    -> *Expression
int Parser::parse(varlisp::Environment& env, const std::string& raw_scripts,
                  bool is_silent)
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, raw_scripts);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, is_silent);

    try {
        // std::cout << "Parser::" << __func__ << "(\"" << scripts << "\")" <<
        // std::endl;

        bool is_balance = true;
        bool do_echo = true;
        sss::string_view ss {raw_scripts};
        sss::trim(ss);
        if (ss.empty()) {
            return 1;
        }
        std::string tmp_scripts;
        if (ss.front() == '@') {
            do_echo = false;
            ss.pop_front();
            tmp_scripts = ss.to_string();
        }
        const std::string& scripts{do_echo ? raw_scripts : tmp_scripts};

        Tokenizer_stat_wrapper(m_toknizer, is_silent);
        // std::string scripts = "(list 12.5 abc 1.2 #f #t xy-z \"123\" )";
        m_toknizer.append(scripts);

        while (is_balance && m_toknizer.top().which() != 0) {
            try {
                if (!this->balance_preread()) {
                    is_balance = false;
                    break;
                }
                auto expr = this->parseExpression();
                // NOTE 这里返回的是一个"表"，也就是lisp中的一个完整的语句。
                // 比如，输入(define x (list + 1 2))，那么得到的expr，也就是这个"表"
                // 此时，我需要对这个"表达式"进行求值。
                // 如果是字面值，则原样。
                // 如果是符号，则会持续求值，直到不是符号。
                // 如果是s-list，则原样；
                // 如果是其他的list或者是可执行的可eval()的类型，则求值。
                // 也就是说，对于 (eval (list 1 2))
                // 这个需求来说，我只需要特化eval_eval函数即可，不用特意修改。
                COLOG_TRIGER_DEBUG(expr);

                Object result;
                const Object& res = varlisp::getAtomicValue(env, expr, result); // varlisp::getAtomicValueUnquote(env, expr, result);
                if (!is_silent && do_echo) {
                    boost::apply_visitor(print_visitor(std::cout), res);
                    std::cout << std::endl;
                }
                env["_"] = result;
            }
            catch (std::runtime_error& e) {
                std::cout << e.what() << std::endl;
                this->m_toknizer.clear();
                return -1;
                // NOTE
                // should return error code
            }
        }
        return is_balance ? 1 : 0;
    }
    catch (const ss1x::parser::ErrorPosition& e) {
        std::cout << e.what() << std::endl;
        this->m_toknizer.clear();
        return -1;
    }
}

int Parser::retrieve_symbols(std::vector<std::string>& symbols,
                             const char* prefix) const
{
    int cnt = 0;
    this->m_toknizer.retrieve_symbols(symbols, prefix);
    for (const auto item : keywords_t::get_keywords_vector()) {
        if (sss::is_begin_with(item.to_string(), prefix)) {
            ++cnt;
            symbols.push_back(item.to_string());
        }
    }
    return cnt;
}

namespace detail {
inline int parenthese_type(parenthese_t pt)
{
    switch (pt) {
        case varlisp::left_parenthese:
        case varlisp::right_parenthese:
            return 0;
        case varlisp::left_bracket:
        case varlisp::right_bracket:
            return 1;
        case varlisp::left_curlybracket:
        case varlisp::right_curlybracket:
            return 2;
        default:
            return -1;
    }
}

} // namespace detail

bool Parser::balance_preread()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    size_t token_cnt = 0;
    std::vector<std::tuple<int, int, int>> pt_stack;
    const static std::tuple<int, int, int> pt_zero;
    std::vector<int> pt_types;
    pt_types.push_back(-1);
    COLOG_TRIGER_DEBUG(SSS_VALUE_MSG(pt_zero));
    // int parenthese_balance = 0;
    // int bracket_balance = 0;
    // int curly_balance = 0;
    for (varlisp::Token tok = this->m_toknizer.lookahead_nothrow(token_cnt);
         tok = this->m_toknizer.lookahead_nothrow(token_cnt), tok.which() != 0; token_cnt++)
    {
        COLOG_TRIGER_DEBUG(tok);
        const varlisp::parenthese_t * p_parenthese = boost::get<varlisp::parenthese_t>(&tok);
        if (p_parenthese == nullptr) {
            continue;
        }

        // 如何判断括号匹配失衡？
        // 1. 左括号随时可以出现——只要当前括号是平衡的，那么三种左括号，都可以出现。
        // 2. 右括号出现的时候，必须在左侧找到"空闲"的左括号；
        // 3. 解析完成(eof)的失衡，必须没有孤立的括号。
        //
        // 简单优化了一下。让同类型的括号可以累加；
        // 一旦有不同类型的括号，就使用栈。
        // 这样，就可以保证括号的配对使用了。

        if (pt_types.back() != detail::parenthese_type(*p_parenthese)) {
            pt_stack.emplace_back(0, 0, 0);
            pt_types.push_back(detail::parenthese_type(*p_parenthese));
        }
        COLOG_TRIGER_DEBUG(SSS_VALUE_MSG(pt_stack.size()), pt_stack);
        switch (*p_parenthese) {
            case varlisp::left_parenthese:
                std::get<0>(pt_stack.back()) ++;
                break;
            case varlisp::right_parenthese:
                std::get<0>(pt_stack.back()) --;
                break;
            case varlisp::left_bracket:
                std::get<1>(pt_stack.back()) ++;
                break;
            case varlisp::right_bracket:
                std::get<1>(pt_stack.back()) --;
                break;
            case varlisp::left_curlybracket:
                std::get<2>(pt_stack.back()) ++;
                break;
            case varlisp::right_curlybracket:
                std::get<2>(pt_stack.back()) --;
                break;
            default:
                SSS_POSITION_THROW(std::runtime_error, "parenthese_t == 0");
        }

        if (pt_stack.back() == pt_zero) {
            pt_stack.pop_back();
            pt_types.pop_back();
        }
        else if (std::get<0>(pt_stack.back()) < 0 ||
                 std::get<1>(pt_stack.back()) < 0 ||
                 std::get<2>(pt_stack.back()) < 0)
        {
            this->m_toknizer.print_token_stack(std::cout);
            this->m_toknizer.clear();
            SSS_POSITION_THROW(std::runtime_error, "parenthese not balance!");
        }
        if (pt_stack.empty()) {
            break;
        }
    }
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, pt_stack.size());
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, token_cnt);
    m_parenthese_stack = pt_zero;
    for (auto item : pt_stack) {
        std::get<0>(m_parenthese_stack) += std::get<0>(item);
        std::get<1>(m_parenthese_stack) += std::get<1>(item);
        std::get<2>(m_parenthese_stack) += std::get<2>(item);
    }
    return pt_stack.empty();
}

// Expression:
//      -> '(' Expression* ')'
//      -> Boolean
//      -> Number
//      -> String
//      -> Symbol
Object Parser::parseExpression()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    varlisp::Token tok = this->m_toknizer.top();
    COLOG_TRIGER_DEBUG(tok);

    if (const varlisp::parenthese_t* p_v =
        boost::get<varlisp::parenthese_t>(&tok)) {
        switch (*p_v) {
            case varlisp::left_parenthese:
                {
                    varlisp::Token tok2 = this->m_toknizer.lookahead(1);
                    const auto * p_k =  boost::get<varlisp::keywords_t>(&tok2);
                    if ((p_k != nullptr) && p_k->type() == keywords_t::kw_CONTEXT) {
                        return this->parseEnvironment();
                    }
                    if ((p_k != nullptr) && p_k->type() == keywords_t::kw_QUOTE) {
                        // (quote ...)形式，只接受一个参数！
                        // > (quote 1 2)
                        // 1
                        return this->parseQuote();
                    }
                    return this->parseList();
                }
                break;

            case varlisp::left_bracket:
                return this->parseList();
                break;

            case varlisp::left_curlybracket:
                return this->parseEnvironment();
                break;

            default:
                SSS_POSITION_THROW(std::runtime_error, "unexpect ", char(*p_v));
        }
    }
    else if (boost::get<varlisp::quote_sign_t>(&tok)) {
        return this->parseQuote();
    }
    else if (const bool* p_v = boost::get<bool>(&tok)) {
        this->m_toknizer.consume();
        return *p_v;
    }
    else if (const int64_t* p_v = boost::get<int64_t>(&tok)) {
        this->m_toknizer.consume();
        return *p_v;
    }
    else if (const double* p_v = boost::get<double>(&tok)) {
        this->m_toknizer.consume();
        return *p_v;
    }
    else if (const std::string* p_v = boost::get<std::string>(&tok)) {
        this->m_toknizer.consume();
        return string_t(*p_v);
    }
    else if (const varlisp::symbol* p_v = boost::get<varlisp::symbol>(&tok)) {
        this->m_toknizer.consume();
        return *p_v;
    }
    else if (const varlisp::keywords_t* p_v = boost::get<varlisp::keywords_t>(&tok)) {
        this->m_toknizer.consume();
        if (p_v->type() == varlisp::keywords_t::kw_NIL) {
            return varlisp::Nill{};
        }
        return *p_v;
    }
    else if (const varlisp::regex_t* p_v = boost::get<varlisp::regex_t>(&tok)) {
        this->m_toknizer.consume();
        return *p_v;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error,
                           "connot handle boost::variant which() == ",
                           tok.which());
    }
}

// { (sym (expr))... }
Object Parser::parseEnvironment()
{
    varlisp::parenthese_t end_pt = varlisp::none_parenthese;
    if (this->m_toknizer.consume(varlisp::left_parenthese)) {
        end_pt = varlisp::right_parenthese;
        if (!this->m_toknizer.consume(varlisp::keywords_t::kw_CONTEXT)) {
            SSS_POSITION_THROW(std::runtime_error, "expect 'context'; but",
                               this->m_toknizer.top());
        }
    }
    else if (this->m_toknizer.consume(varlisp::left_curlybracket)) {
        end_pt = varlisp::right_curlybracket;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "expect '(context' or '{'");
    }
    Environment env;
    // NOTE eval时机问题；
    // 让内部的对象，通过这个env进行eval；

    varlisp::Token tok;
    while (tok = m_toknizer.top(), tok.which() != 0) {
        COLOG_TRIGER_DEBUG(tok);
        if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
            break;
        }
        tok = m_toknizer.top();
        const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&tok);
        if (p_sym == nullptr) {
            SSS_POSITION_THROW(std::runtime_error, "expected symbol; but ", tok);
        }
        this->m_toknizer.consume();
        env[p_sym->name()] = this->parseExpression();
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSITION_THROW(std::runtime_error, "expect ')'");
        }
    }
    if (!this->m_toknizer.consume(end_pt)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ", sss::raw_char(char(end_pt)));
    }
    return std::move(env);
    // return env;
}

Object Parser::parseQuote()
{
    if (this->m_toknizer.consume(varlisp::quote_sign_t{})) {
        Object value = this->parseExpression();
        COLOG_TRIGER_DEBUG(value);
        return varlisp::List::makeSQuoteObj(std::move(value));
    }
    if (this->m_toknizer.consume(varlisp::left_parenthese)) {
        if (!this->m_toknizer.consume(varlisp::keywords_t::kw_QUOTE)) {
            SSS_POSITION_THROW(std::runtime_error, "expect (quote ..., but only ", this->m_toknizer.top(), " after (");
        }
        Object value = this->parseExpression();
        COLOG_TRIGER_DEBUG(value);
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSITION_THROW(std::runtime_error, "expect ) but ", this->m_toknizer.top());
        }
        return varlisp::List::makeSQuoteObj(value);
    }
    SSS_POSITION_THROW(std::runtime_error, "expect ' or (quote ..., but ", this->m_toknizer.top());
}

// FIXME
//
// '(' 应该由各自的函数自己进行consume!
//
// > (quote (1 2))
// (1 2)
// > (list 1 2)
// (1 2)
// NOTE quote 和list等效；区别只是……
Object Parser::parseList()
{
    COLOG_TRIGER_DEBUG(this->m_toknizer.top());
    varlisp::parenthese_t end_pt = varlisp::none_parenthese;
    if (this->m_toknizer.consume(varlisp::left_parenthese)) {
        end_pt = varlisp::right_parenthese;

        varlisp::Token next = this->m_toknizer.lookahead(0);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, next);

        varlisp::keywords_t * p_k = boost::get<varlisp::keywords_t>(&next);
        if (p_k != nullptr) {
            switch (p_k->type()) {
                case keywords_t::kw_IF:
                    return this->parseSpecialIf();

                case keywords_t::kw_COND:
                    return this->parseSpecialCond();

                case keywords_t::kw_AND:
                    return this->parseSpecialAnd();

                case keywords_t::kw_OR:
                    return this->parseSpecialOr();

                case keywords_t::kw_DEFINE:
                    return this->parseSpecialDefine();

                case keywords_t::kw_LAMBDA:
                    return this->parseSpecialLambda();

                case keywords_t::kw_LIST:
                    break;

                default:
                    COLOG_TRIGER_INFO(p_k->name());
            }
        }
    }
    else if (this->m_toknizer.consume(varlisp::left_bracket)) {
        end_pt = varlisp::right_bracket;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "expect '(' or '['");
    }
    // NOTE (1 2 3) (list 1 2 3)
    // 的区别，仅在于第一个元素的不同；
    // 后者的第一个元素，相当于一个函数；其作用，就是将当前列表之后的部分整体返回！
    // 可以看做：(cdr (list list 1 2 3))

    varlisp::List current;

    detail::list_back_inserter_t<detail::Converter<Object> > inserter(current);

    varlisp::Token tok;

    bool is_quote_list = this->m_toknizer.consume(varlisp::keywords_t::kw_LIST);

    COLOG_TRIGER_DEBUG(SSS_VALUE_MSG(is_quote_list));

    while (tok = m_toknizer.top(), tok.which() != 0) {
        COLOG_TRIGER_DEBUG(current);
        switch (tok.which()) {
            case 0:
                break;

            case 1:
                if (tok == Token(varlisp::left_parenthese) || tok == Token(varlisp::left_bracket)) {
                    *inserter++ = this->parseList();
                }
                else if (tok == Token(varlisp::left_curlybracket)) {
                    *inserter++ = this->parseEnvironment();
                }
                else {
                    if (!this->m_toknizer.consume(end_pt)) {
                        SSS_POSITION_THROW(std::runtime_error, "expect ", char(end_pt));
                    }
                    if (end_pt == varlisp::right_bracket) {
                        return varlisp::List::makeSQuoteObj(current);
                    }
                    if (is_quote_list) {
                        return varlisp::List::makeSQuoteObj(current);
                    }

                    return current;
                }
                break;

            case 2:
                *inserter++ = boost::get<bool>(tok);
                m_toknizer.consume();
                break;

            case 3:
                *inserter++ = boost::get<int64_t>(tok);
                m_toknizer.consume();
                break;

            case 4:
                *inserter++ = boost::get<double>(tok);
                m_toknizer.consume();
                break;

            case 5:
                *inserter++ = varlisp::string_t(boost::get<std::string>(tok));
                m_toknizer.consume();
                break;

            case 6:
                {
                    COLOG_TRIGER_DEBUG(SSS_VALUE_MSG(tok));
                    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&tok);
                    *inserter++ = *p_sym;
                }
                m_toknizer.consume();
                break;

            case 7:
                {
                    *inserter++ = boost::get<varlisp::regex_t>(tok);
                    m_toknizer.consume();
                }
                break;

            case 8:
                {
                    auto key = boost::get<varlisp::keywords_t>(tok);
                    if (key.type() == keywords_t::kw_NIL) {
                        *inserter++ = varlisp::Nill{};
                    }
                    else {
                        *inserter++ = key;
                    }
                    m_toknizer.consume();
                }
                break;

            case 9:
                *inserter++ = this->parseQuote();
                break;

            default:
                {
                    SSS_POSITION_THROW(std::runtime_error, "unexpect token:", tok);
                }
                break;
        }
    }

    return {};
}

Object Parser::parseSpecialIf()
{
    if (!this->m_toknizer.consume(keywords_t::kw_IF)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'if'");
    }
    Object condition = parseExpression();
    Object consequent = parseExpression();
    Object alternative = varlisp::Nill{};
    if (this->m_toknizer.lookahead(0) != Token(varlisp::right_parenthese)) {
        alternative = parseExpression();
    }
    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ')'");
    }

    return varlisp::IfExpr(condition, consequent, alternative);;
}

Object Parser::parseSpecialCond()
{
    if (!this->m_toknizer.consume(keywords_t::kw_COND)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'cond'; but",
                           this->m_toknizer.lookahead(0));
    }
    bool has_else_clause = false;
    std::vector<std::pair<Object, Object>> conditions;

    while (!has_else_clause) {
        if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
            break;
        }
        Object predict = this->parseExpression();
        Object expr = this->parseExpression();
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            std::ostringstream oss;
            // this->m_toknizer.print(oss);
            this->m_toknizer.print_token_stack(oss);
            SSS_POSITION_THROW(std::runtime_error, "expect ')'; but ",
                               this->m_toknizer.lookahead(0), oss.str());
        }
        conditions.emplace_back(predict, expr);
        if (const auto* pv = boost::get<varlisp::keywords_t>(&predict)) {
            if (pv->type() == keywords_t::kw_ELSE) {
                has_else_clause = true;
            }
        }
    }
    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ')'; but ",
                           this->m_toknizer.lookahead(0));
    }

    return varlisp::Cond(conditions);
}

Object Parser::parseSpecialAnd()
{
    if (!this->m_toknizer.consume(keywords_t::kw_AND)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'and'; but ",
                           this->m_toknizer.lookahead(0));
    }

    std::vector<Object> conditions;

    while (true) {
        varlisp::Token tok = this->m_toknizer.lookahead(0);
        if (tok == varlisp::Token(varlisp::right_parenthese)) {
            break;
        }
        conditions.emplace_back(this->parseExpression());
    }
    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ')'");
    }

    return varlisp::LogicAnd(conditions);
}

Object Parser::parseSpecialOr()
{
    if (!this->m_toknizer.consume(keywords_t::kw_OR)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'or'");
    }
    std::vector<Object> conditions;

    while (true) {
        varlisp::Token tok = this->m_toknizer.lookahead(0);
        if (tok == varlisp::Token(varlisp::right_parenthese)) {
            break;
        }
        conditions.emplace_back(this->parseExpression());
    }
    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ')'");
    }

    return varlisp::LogicOr(conditions);
}

int parseParamVector(varlisp::Tokenizer& toknizer,
                     std::vector<std::string>& args)
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    size_t old_len = args.size();
    while (true) {
        Token tok = toknizer.lookahead();
        auto *sym = boost::get<varlisp::symbol>(&tok);
        if (sym == nullptr) {
            break;
        }
        const std::string& name = sym->name();
        args.push_back(name);
        toknizer.consume();
    }
    return int(args.size() - old_len);
}

// (define symbol expr boolean-expr)
// (define (symbol arg-list) "doc" (expr-list))
Object Parser::parseSpecialDefine()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    if (!this->m_toknizer.consume(keywords_t::kw_DEFINE)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'define'; but ", this->m_toknizer.top());
    }

    varlisp::Token tok = this->m_toknizer.lookahead();

    if (const varlisp::symbol* p_name = boost::get<varlisp::symbol>(&tok)) {
        this->m_toknizer.consume();
        varlisp::Object value = this->parseExpression();

        varlisp::Object force;
        if (this->m_toknizer.lookahead(0) != Token(varlisp::right_parenthese)) {
            force = this->parseExpression();
        }

        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSITION_THROW(std::runtime_error, "expect ')'; but",
                               this->m_toknizer.lookahead(0));
        }
        if (force.which() != 0) {
            return Define(*p_name, value, force);
        }
        return Define(*p_name, value);
    }
    else if (const varlisp::parenthese_t* p_v =
             boost::get<varlisp::parenthese_t>(&tok)) {
        if (*p_v != varlisp::left_parenthese) {
            SSS_POSITION_THROW(std::runtime_error, "expect '('");
        }
        this->m_toknizer.consume();

        tok = this->m_toknizer.lookahead();
        if (varlisp::symbol* p_symbol = boost::get<varlisp::symbol>(&tok)) {
            this->m_toknizer.consume();

            std::vector<std::string> args;
            parseParamVector(m_toknizer, args);

            if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                SSS_POSITION_THROW(std::runtime_error, "expect ')'; but ",
                                   this->m_toknizer.lookahead(0));
            }

            varlisp::string_t help_msg;
            varlisp::Token firstTok = this->m_toknizer.lookahead(0);
            const std::string * p_msg = boost::get<std::string>(&firstTok);
            if ((p_msg != nullptr) && this->m_toknizer.lookahead(1) != Token(varlisp::right_parenthese)) {
                help_msg = varlisp::string_t(*p_msg);
                this->m_toknizer.consume();
            }

            std::vector<Object> body;
            while (true) {
                varlisp::Token tok = this->m_toknizer.lookahead(0);
                if (tok == varlisp::Token(varlisp::right_parenthese)) {
                    break;
                }
                body.push_back(this->parseExpression());
            }
            if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                SSS_POSITION_THROW(std::runtime_error, "expect ')'");
            }

            varlisp::Lambda lambda(std::move(args), std::move(help_msg), std::move(body));

            return varlisp::Define(*p_symbol, lambda);
        }
        SSS_POSITION_THROW(std::runtime_error, "expect a valid func-name");
    }
    return Nill{};
}

// (labmda (arg-list) "" expr-list)
Object Parser::parseSpecialLambda()
{
    if (!this->m_toknizer.consume(keywords_t::kw_LAMBDA)) {
        SSS_POSITION_THROW(std::runtime_error, "expect 'lambda'");
    }
    std::vector<std::string> args;
    if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect '('");
    }

    parseParamVector(m_toknizer, args);

    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        std::ostringstream oss;
        this->m_toknizer.print_token_stack(oss);
        SSS_POSITION_THROW(std::runtime_error, "expect ')' but ", this->m_toknizer.lookahead(0), oss.str());
    }

    varlisp::string_t help_msg;
    varlisp::Token firstTok = this->m_toknizer.lookahead(0);
    const std::string * p_msg = boost::get<std::string>(&firstTok);
    if ((p_msg != nullptr) && this->m_toknizer.lookahead(1) != Token(varlisp::right_parenthese)) {
        help_msg = varlisp::string_t(*p_msg);
        this->m_toknizer.consume();
    }

    std::vector<Object> body;
    while (true) {
        varlisp::Token tok = this->m_toknizer.lookahead(0);
        if (tok == varlisp::Token(varlisp::right_parenthese)) {
            break;
        }
        // NOTE Lambda的body部分，可以有很多语句；
        body.push_back(this->parseExpression());
    }

    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
        SSS_POSITION_THROW(std::runtime_error, "expect ')'; but",
                           this->m_toknizer.lookahead(0));
    }
    return varlisp::Lambda(std::move(args), std::move(help_msg), std::move(body));
}

// '(' "eval" Expression+ ')'
// lisp是一个包含上下文的语言——它可以载入各种文法，以形成新的解析形式；
//! https://zh.wikipedia.org/wiki/Eval
// (eval (read-from-string "(format t \"Hello World!!!~%\")"))
//
// 上例中，字符串，被read-from-string函数，解析为list；然后，由eval，对其进行解释；
//
// 又如：
//
// (define form2 '(+ 5 2))
//
// (eval form2 user-initial-environment)
// (eval form2)
//
// 其中， user-initial-environment 用来指定解析的上下文；
//
// 重定义 符号+ 的行为 为 "-"
// ;; Confuse the initial environment, so that + will be
// ;; a name for the subtraction function.
// (environment-define user-initial-environment '+ -)
// ;Value: +
//
//! http://clhs.lisp.se/Body/f_eval.htm
//
// > (eval (list + 1 2))
// 3
// > (eval list + 1 2))
// eval: arity mismatch;
//  the expected number of arguments does not match the given number
//   expected: 1 to 2
//   given: 4
//   arguments...:
//    #<procedure:list>
//    #<procedure:+>
//    1
//    2
// > (eval 1)
// 1
// 可见，eval也相当于一个内建的函数……
// 它需要1-2个参数；分别是一个表达式；还有一个是执行的环境(可选)；
// 这样来看的话，我也没有必要将其特殊化了；直接弄进builtin就好了。

}  // namespace varlisp
