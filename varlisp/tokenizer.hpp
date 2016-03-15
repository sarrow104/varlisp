#ifndef __TOKENIZER_HPP_1457185659__
#define __TOKENIZER_HPP_1457185659__

#include <string>
#include <vector>

#include <iostream>

#include <boost/variant.hpp>

#include <ss1x/parser/oparser.hpp>

#include "symbol.hpp"

namespace varlisp {

    struct empty{
        bool operator == (const empty& ) const {
            return true;
        }
    };

    enum parenthese_t {
        left_parenthese     = '(',
        right_parenthese    = ')'
    };

    typedef boost::variant<empty,           // 0
                           parenthese_t,    // 1 括号
                           bool,            // 2 #t,#f 字符量
                           int,             // 3 integer 字符量
                           double,          // 4
                           std::string,     // 5 保存去掉括号后的字符串
                           symbol>          // 6 符号(包括关键字、运算符)
    Token;

    class token_visitor  :  public boost::static_visitor<void>
    {
    private:
        std::ostream& m_o;
    public:
        token_visitor(std::ostream& o)
            : m_o(o)
        {
        }

    public:
        void operator()(const empty& ) const
        {
            m_o << "<empty>";
        }

        void operator() (bool b) const
        {
            m_o << (b ? "true" : "false");
        }
        void operator()(int i) const
        {
            m_o << i ;
        }
        void operator()(const std::string& s) const
        {
            m_o << '"' << s << '"';
        }
        void operator()(double d) const
        {
            m_o << d;
        }
        void operator()(const symbol& s ) const
        {
            m_o << s.m_data;
        }
        void operator() (parenthese_t p) const
        {
            m_o << (p == varlisp::left_parenthese ? '(' : ')');
        }
    };

    inline std::ostream& operator << (std::ostream& o, const Token& tok)
    {
        token_visitor tv(o);
        boost::apply_visitor(tv, tok);
        return o;
    }

    class Tokenizer
    {
        typedef std::string::const_iterator StrIterator;
    public:
        Tokenizer();

        explicit Tokenizer(const std::string& data);

        ~Tokenizer() = default;

    public:
        Tokenizer(Tokenizer&& ) = default;
        Tokenizer& operator = (Tokenizer&& ) = default;

    public:
        Tokenizer(const Tokenizer& ) = default;
        Tokenizer& operator = (const Tokenizer& ) = default;

    public:
        Token   top();
        bool    empty() const;

        Token   lookahead_nothrow(int index = 0);
        Token   lookahead(int index = 0);

        bool    consume();

        bool    consume(parenthese_t paren);

        bool    consume(const symbol& symbol);

        int     consume_count() const
        {
            return this->m_consumed;
        }

    public:
        void    append(const std::string& scripts);

        size_t  tokens_size() const;
        bool    is_eof() const;

        void    clear();

        int     retrieve_symbols(std::vector<std::string>& symbols, const char * prefix) const;

    public:
        void    push();
        void    pop();

    protected:
        void    init();
        void    parse();

    private:
        int                 m_consumed;
        std::string         m_data;
        StrIterator         m_beg;
        StrIterator         m_end;

        Token               tok;
        std::string         str_stack;
        ss1x::parser::rule  Comment_p;
        ss1x::parser::rule  Spaces_p;
        ss1x::parser::rule  TokenEnd_p;
        ss1x::parser::rule  Integer_p;
        ss1x::parser::rule  Double_p;
        ss1x::parser::rule  Symbol_p;

        ss1x::parser::rule  c_str_escapseq_p;
        ss1x::parser::rule  String_p;
        ss1x::parser::rule  RawString_p;
        ss1x::parser::rule  LeftParen_p;
        ss1x::parser::rule  RightParent_p;

        ss1x::parser::rule  BoolTure_p;
        ss1x::parser::rule  BoolFalse_p;
        ss1x::parser::rule  Token_p;

        std::vector<Token>  m_tokens;

        int                 m_swap_consumed;
        std::string         m_swap_data;
        StrIterator         m_swap_beg;
        StrIterator         m_swap_end;
        std::vector<Token>  m_swap_tokens;
    };

} // namespace varlisp


#endif /* __TOKENIZER_HPP_1457185659__ */
