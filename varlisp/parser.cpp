
#include <sstream>

#include <sss/log.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sss/util/Memory.hpp>

#include <ss1x/parser/oparser.hpp>

#include "tokenizer.hpp"
#include "parser.hpp"
#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"

namespace varlisp {

    class Tokenizer_stat_wrapper
    {
    public:
        Tokenizer&  m_tz;
        bool        m_active;
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

    // scripts
    //    -> *Expression
    int Parser::parse(varlisp::Environment& env, const std::string& scripts, bool is_silent)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, scripts);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, is_silent);

        // std::cout << "Parser::" << __func__ << "(\"" << scripts << "\")" << std::endl;

        bool is_balance = true;

        Tokenizer_stat_wrapper(m_toknizer, is_silent);
        // std::string scripts = "(list 12.5 abc 1.2 #f #t xy-z \"123\" )";
        m_toknizer.append(scripts);

        while (is_balance && (m_toknizer.top().which()))
        {
            try {
                if (!this->balance_preread()) {
                    is_balance = false;
                    break;
                }
                auto expr = this->parseExpression();
                const auto res = boost::apply_visitor(eval_visitor(env), expr);
                // std::cout << expr << " = " << res << std::endl;
                if (!is_silent && res.which()) {
#if 1
                    boost::apply_visitor(print_visitor(std::cout), res);
                    std::cout << std::endl;
#else
                    std::cout << res.which() << " " << res << std::endl;
#endif
                }
            }
            catch (std::runtime_error& e) {
                std::cout << e.what() << std::endl;
                this->m_toknizer.clear();
                return -1;
                // NOTE
                // should return error code
            }
        }
        return is_balance;
    }

    int Parser::retrieve_symbols(std::vector<std::string>& symbols, const char * prefix) const
    {
        this->m_toknizer.retrieve_symbols(symbols, prefix);
        const char * keywords[] = {
            "if",
            "else",
            "cond",
            "and",
            "or",
            "define",
            "lambda",
        };
        for (const auto * item : keywords) {
            if (sss::is_begin_with(item, prefix)) {
                symbols.push_back(item);
            }
        }
    }

    bool Parser::balance_preread()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        size_t token_cnt = 0;
        int parenthese_balance = 0;
        while (true) {
            static const varlisp::Token left(varlisp::left_parenthese);
            static const varlisp::Token right(varlisp::right_parenthese);
            varlisp::Token tok = this->m_toknizer.lookahead_nothrow(token_cnt);
            if (!tok.which()) {
                break;
            }
            if (tok == left) {
                parenthese_balance ++;
            }
            if (tok == right) {
                parenthese_balance --;
            }
            if (parenthese_balance < 0) {
                this->m_toknizer.print_token_stack(std::cout);
                this->m_toknizer.clear();
                SSS_POSTION_THROW(std::runtime_error,
                                  "parenthese not balance!");
            }
            if (parenthese_balance == 0) {
                break;
            }
            token_cnt++;
        }
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, parenthese_balance);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, token_cnt);
        return parenthese_balance == 0;
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

        if (const varlisp::parenthese_t * p_v = boost::get<varlisp::parenthese_t>(&tok)) {
            if (*p_v != varlisp::left_parenthese) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "unexpect ')'");
            }
            return this->parseList();
        }
        else if (const bool * p_v = boost::get<bool>(&tok)) {
            this->m_toknizer.consume();
            return varlisp::Object(*p_v);
        }
        else if (const int * p_v = boost::get<int>(&tok)) {
            this->m_toknizer.consume();
            return varlisp::Object(*p_v);
        }
        else if (const double * p_v = boost::get<double>(&tok)) {
            this->m_toknizer.consume();
            return varlisp::Object(*p_v);
        }
        else if (const std::string * p_v = boost::get<std::string>(&tok)) {
            this->m_toknizer.consume();
            return varlisp::Object(*p_v);
        }
        else if (const varlisp::symbol * p_v = boost::get<varlisp::symbol>(&tok)) {
            this->m_toknizer.consume();
            return varlisp::Object(*p_v);
        }
        else {
            SSS_POSTION_THROW(std::runtime_error,
                              "connot handle boost::variant which() == "
                              , tok.which());
        }
    }

    // FIXME
    //
    // '(' 应该由各自的函数自己进行consume!
    //
    Object Parser::parseList()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect '('");
        }

        varlisp::Token next = this->m_toknizer.lookahead(0);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, next);

#define define_builtin_symbol(a) static const varlisp::Token tok_##a(varlisp::symbol(#a));
        define_builtin_symbol(if);
        define_builtin_symbol(cond);
        define_builtin_symbol(and);
        define_builtin_symbol(or);
        define_builtin_symbol(define);
        define_builtin_symbol(lambda);
#undef  define_builtin_symbol

        if (next == tok_if) {
            return this->parseSpecialIf();
        }
        else if (next == tok_cond) {
            return this->parseSpecialCond();
        }
        else if (next == tok_and) {
            return parseSpecialAnd();
        }
        else if (next == tok_or) {
            return parseSpecialOr();
        }
        else if (next == tok_define) {
            return this->parseSpecialDefine();
        }
        else if (next == tok_lambda) {
            return this->parseSpecialLambda();
        }
        // NOTE (1 2 3) (list 1 2 3)
        // 的区别，仅在于第一个元素的不同；
        // 后者的第一个元素，相当于一个函数；其作用，就是将当前列表之后的部分整体返回！
        // 可以看做：(cdr (list list 1 2 3))
        // else if (next == tok_list) {
        //     // TODO
        //     return this->parseSpecialList();
        // }

        List current;
        varlisp::Token tok;

        while (tok = m_toknizer.top(), tok.which())
        {
            switch(tok.which()) {
            case 0:
                break;

            case 1:
                if (tok == Token(varlisp::left_parenthese)) {
                   current.append(Object(this->parseList()));
                }
                else {
                    if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                        SSS_POSTION_THROW(std::runtime_error,
                                          "expect ')'");
                    }
                    return current;
                }
                break;

            case 2:
                current.append(Object(boost::get<bool>(tok)));
                m_toknizer.consume();
                break;

            case 3:
                current.append(Object(boost::get<int>(tok)));
                m_toknizer.consume();
                break;

            case 4:
                current.append(Object(boost::get<double>(tok)));
                m_toknizer.consume();
                break;

            case 5:
                current.append(Object(boost::get<std::string>(tok)));
                m_toknizer.consume();
                break;

            case 6:
                current.append(Object(boost::get<varlisp::symbol>(tok)));
                m_toknizer.consume();
                break;
            }
        }

        return Object();
    }

    Object Parser::parseSpecialIf()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("if"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'if'");
        }
        Object condition = parseExpression(); 
        Object consequent = parseExpression();
        Object alternative = parseExpression();

        Object ret = varlisp::IfExpr(condition, consequent, alternative);

        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }
        return ret;
    }

    Object Parser::parseSpecialCond()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("cond"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'cond'");
        }
        bool has_else_clause = false;
        std::vector<std::pair<Object, Object> > conditions;

        while (true) {
            varlisp::Token tok = this->m_toknizer.lookahead(0);
            if (!(tok == varlisp::Token(varlisp::left_parenthese))) {
                break;
            }
            this->m_toknizer.consume();
            Object predict  = this->parseExpression();
            Object expr     = this->parseExpression();
            conditions.emplace_back(predict, expr);
            if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "expect ')'");
            }
            if (const varlisp::symbol * pv = boost::get<varlisp::symbol>(&predict)) {
                if (*pv == varlisp::symbol("else")) {
                    has_else_clause = true;
                    break;
                }
            }
        }
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }

        return varlisp::Cond(conditions);
    }

    Object Parser::parseSpecialAnd()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("and"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'and'");
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
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }

        return varlisp::LogicAnd(conditions);
    }

    Object Parser::parseSpecialOr()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("or"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'or'");
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
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }

        return varlisp::LogicOr(conditions);
    }

    int parseParamVector(varlisp::Tokenizer& toknizer, std::vector<std::string>& args)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        int old_len = args.size();
        while (true) {
            Token tok = toknizer.lookahead();
            if (!boost::get<varlisp::symbol>(&tok)) {
                break;
            }
            const std::string& name = boost::get<varlisp::symbol>(tok).m_data;
            if (!std::isalpha(name[0])) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "we need a variable name here, not `" , name , "`");
            }
            args.push_back(name);
            toknizer.consume();
        }
        return args.size() - old_len;
    }

    Object Parser::parseSpecialDefine()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        if (!this->m_toknizer.consume(varlisp::symbol("define"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'quote'");
        }

        varlisp::Token tok = this->m_toknizer.lookahead();

        if (const varlisp::symbol *p_name = boost::get<varlisp::symbol>(&tok)) {
            if (!std::isalpha(p_name->m_data[0])) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "we need a variable name here, not `"
                                  , p_name->m_data , "`");
            }
            this->m_toknizer.consume();
            varlisp::Object value = this->parseExpression();

            if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "expect ')'");
            }

            return Define(*p_name, value);
        }
        else if (const varlisp::parenthese_t *p_v = boost::get<varlisp::parenthese_t>(&tok)) {
            if (*p_v != varlisp::left_parenthese) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "expect '('");
            }
            this->m_toknizer.consume();

            tok = this->m_toknizer.lookahead();
            if (varlisp::symbol * p_symbol = boost::get<varlisp::symbol>(&tok)) {
                if (!std::isalpha(p_symbol->m_data[0])) {
                    SSS_POSTION_THROW(std::runtime_error,
                                      "we need a variable name here, not `"
                                      , p_symbol->m_data , "`");
                }
                this->m_toknizer.consume();

                std::vector<std::string> args;
                parseParamVector(m_toknizer, args);

                if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
                    SSS_POSTION_THROW(std::runtime_error,
                                      "expect ')'");
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
                    SSS_POSTION_THROW(std::runtime_error,
                                      "expect ')'");
                }
                varlisp::Lambda lambda(std::move(args), std::move(body));

                return varlisp::Define(*p_symbol, lambda);
            }
            else {
                SSS_POSTION_THROW(std::runtime_error,
                                  "expect a valid func-name");
            }
        }
    }

    Object Parser::parseSpecialLambda()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("lambda"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'lambda'");
        }
        std::vector<std::string> args;
        if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect '('");
        }

        parseParamVector(m_toknizer, args);

        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }
        std::vector<Object> body;
        while (true) {
            varlisp::Token tok = this->m_toknizer.lookahead(0);
            if (tok == varlisp::Token(varlisp::right_parenthese)) {
                break;
            }
            body.push_back(this->parseExpression());
        }
        // // 貌似 Lambda的body部分，可以有很多语句；
        // varlisp::Object body = this->parseList();
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }
        // return varlisp::Lambda(args, body);
        return varlisp::Lambda(std::move(args), std::move(body));
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
    // 其中， user-initial-environment 用来制定解析的上下文；
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

} // namespace varlisp
