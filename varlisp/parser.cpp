
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
    void Parser::parse(varlisp::Environment& env, const std::string& scripts)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_ERROR);

        // std::string scripts = "(list 12.5 abc 1.2 #f #t xy-z \"123\" )";
        m_toknizer.append(scripts);
        std::cout << "Parser::" << __func__ << "(\"" << scripts << "\")" << std::endl;

        varlisp::Token tok;

        while (tok = m_toknizer.top(), tok.which())
        {
            try {
                auto node = this->parseExpression();
                const auto res = boost::apply_visitor(eval_visitor(env), node);
                boost::apply_visitor(print_visitor(std::cout), res);
                std::cout << std::endl;
            }
            catch (std::runtime_error& e) {
                std::cout << e.what() << std::endl;
                this->m_toknizer.clear();
            }
        }
    }

    // Expression:
    //      -> '(' Expression* ')'
    //      -> Boolean
    //      -> Number
    //      -> String
    //      -> Symbol
    Object Parser::parseExpression()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_ERROR);
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
        SSS_LOG_FUNC_TRACE(sss::log::log_ERROR);
        if (!this->m_toknizer.consume(varlisp::left_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect '('");
        }

        varlisp::Token next = this->m_toknizer.lookahead(0);
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, next);

#define define_builtin_symbol(a) static const varlisp::Token tok_##a(varlisp::symbol(#a));
        define_builtin_symbol(if);
        define_builtin_symbol(quote);
        define_builtin_symbol(define);
        define_builtin_symbol(lambda);
        define_builtin_symbol(eval);
        define_builtin_symbol(list);
#undef  define_builtin_symbol

        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_list));
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_if));
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_quote));
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_define));
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_lambda));
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, (next == tok_eval));

        if (next == tok_if) {
            return this->parseSpecialIf();
        }
        else if (next == tok_quote) {
            return this->parseSpecialQuote();
        }
        else if (next == tok_define) {
            return this->parseSpecialDefine();
        }
        else if (next == tok_lambda) {
            return this->parseSpecialLambda();
        }
        else if (next == tok_eval) {
            return this->parseSpecialEval();
        }
        else if (next == tok_list) {
            return this->parseSpecialList();
        }

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
        return varlisp::IfExpr(parseExpression(), parseExpression(), parseExpression());
    }

    Object Parser::parseSpecialQuote()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("quote"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'quote'");
        }
        throw std::runtime_error("not implement parseSpecialQuote");
        return Object();
    }

    Object Parser::parseSpecialDefine()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_ERROR);
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
            if (tok.which() != 6) {
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
        varlisp::Object body = this->parseList();
        if (!this->m_toknizer.consume(varlisp::right_parenthese)) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect ')'");
        }
        // return varlisp::Lambda(args, body);
        return varlisp::Lambda(std::move(args), std::move(body));
    }

    Object Parser::parseSpecialEval()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("eval"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'eval'");
        }
    }

    Object Parser::parseSpecialList()
    {
        if (!this->m_toknizer.consume(varlisp::symbol("list"))) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect 'list'");
        }
    }

    Object Parser::parseSymbol()
    {
        return 0;
    }

    Object Parser::parseNumber()
    {
        return 0;
    }

    Object Parser::parseDouble()
    {
        return 0;
    }

    Object Parser::parseInteger()
    {
        return 0;
    }

    Object Parser::parseString()
    {
        return 0;
    }

} // namespace varlisp
