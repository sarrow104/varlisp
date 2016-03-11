#ifndef __TOKENIZER_HPP_1457185659__
#define __TOKENIZER_HPP_1457185659__

#include <string>
#include <vector>

#include <iostream>

#include <boost/variant.hpp>

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

    public:

    protected:
        void    parse();

    private:
        int                 m_consumed;
        std::string         m_data;
        StrIterator         m_beg;
        StrIterator         m_end;

        std::vector<Token>  m_tokens;
    };

} // namespace varlisp


#endif /* __TOKENIZER_HPP_1457185659__ */
