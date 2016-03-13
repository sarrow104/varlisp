
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

    // scripts
    //    -> *Expression
    int Parser::parse(varlisp::Environment& env, const std::string& scripts)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, scripts);

        // std::string scripts = "(list 12.5 abc 1.2 #f #t xy-z \"123\" )";
        m_toknizer.append(scripts);

        // std::cout << "Parser::" << __func__ << "(\"" << scripts << "\")" << std::endl;

        bool is_balance = true;

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
                if (res.which()) {
                    std::cout << res << std::endl;
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

    bool Parser::balance_preread()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        size_t token_cnt = 0;
        int parenthese_balance = 0;
        while (true) {
            static varlisp::Token left(varlisp::left_parenthese);
            static varlisp::Token right(varlisp::right_parenthese);
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
        varlisp::Token tok;
        while (tok = this->m_toknizer.top(), tok.which())
        {
            switch(tok.which()) {
            case 0:
                break;

            case 1:
                if (tok == Token(varlisp::left_parenthese)) {
                    return this->parseList();
                }
                else {
                    SSS_POSTION_THROW(std::runtime_error,
                                      "unexpect ')'");
                }
                break;

            case 2:
                this->m_toknizer.consume();
                return varlisp::Object(boost::get<bool>(tok));

            case 3:
                this->m_toknizer.consume();
                return varlisp::Object(boost::get<int>(tok));

            case 4:
                this->m_toknizer.consume();
                return varlisp::Object(boost::get<double>(tok));

            case 5:
                this->m_toknizer.consume();
                // TODO 需要去掉引号，处理转义
                return varlisp::Object(boost::get<std::string>(tok));

            case 6:
                this->m_toknizer.consume();
                // symbol
                return varlisp::Object(boost::get<varlisp::symbol>(tok));
            }
        }
        return varlisp::Object();
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
        define_builtin_symbol(define);
        define_builtin_symbol(lambda);
#undef  define_builtin_symbol

        if (next == tok_if) {
            return this->parseSpecialIf();
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
        Object alternative;

        varlisp::Token tok = this->m_toknizer.lookahead();
        if (!(tok == varlisp::Token(varlisp::right_parenthese))) {
            alternative = parseExpression();
        }

        Object ret = varlisp::IfExpr(condition, consequent, alternative);

        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }
        return ret;
    }

    Object Parser::parseSpecialDefine()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        if (!this->m_toknizer.consume(varlisp::symbol("define"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'quote'");
        }

        varlisp::symbol name;
        try {
            name = boost::get<varlisp::symbol>(this->m_toknizer.lookahead());
        }
        catch (const boost::bad_get& e) {
            SSS_POSTION_THROW(std::runtime_error,
                              "we need a variable name here, not `" << this->m_toknizer.lookahead() << "`");
        }
        if (!std::isalpha(name.m_data[0])) {
            SSS_POSTION_THROW(std::runtime_error,
                              "we need a variable name here, not `" << name.m_data << "`");
        }
        this->m_toknizer.consume();
        varlisp::Object value = this->parseExpression();

        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }

        return Define(name, value);
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
        while (true) {
            Token tok = this->m_toknizer.lookahead();
            if (!boost::get<varlisp::symbol>(&tok)) {
                break;
            }
            const std::string& name = boost::get<varlisp::symbol>(tok).m_data;
            if (!std::isalpha(name[0])) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "we need a variable name here, not `" << name << "`");
            }
            args.push_back(name);
            this->m_toknizer.consume();
        }
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
            // std::cout << body << std::endl;
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
