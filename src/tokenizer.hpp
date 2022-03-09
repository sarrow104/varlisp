#ifndef __TOKENIZER_HPP_1457185659__
#define __TOKENIZER_HPP_1457185659__

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <stdexcept>

#include <boost/variant.hpp>

#include <ss1x/parser/oparser.hpp>

#include <sss/raw_print.hpp>
#include <sss/util/PostionThrow.hpp>

#include "regex_t.hpp"
#include "symbol.hpp"
#include "keyword_t.hpp"

namespace detail {
bool & tokenizer_colog_switch();
} // namespace detail

namespace varlisp {

struct empty{
    bool operator == (const empty& ) const {
        return true;
    }
};

struct quote_sign_t {
    bool operator == (const quote_sign_t& ) const {
        return true;
    }
};

enum parenthese_t {
    none_parenthese    = '\0',
    left_parenthese    = '(',
    right_parenthese   = ')',
    left_bracket       = '[',
    right_bracket      = ']',
    left_curlybracket  = '{',
    right_curlybracket = '}',
};

typedef boost::variant<empty,   // 0
        parenthese_t,           // 1 括号
        bool,                   // 2 #t,#f 字符量
        int64_t,                // 3 integer 字符量
        double,                 // 4
        std::string,            // 5 保存去掉括号后的字符串
        varlisp::symbol,        // 6 符号(包括运算符)
        varlisp::regex_t,       // 7 正则表达式
        varlisp::keywords_t,    // 8 关键字（包括关键字）
        quote_sign_t            // 9 单引号
        >
        Token;

class token_print_visitor : public boost::static_visitor<void> {
private:
    std::ostream& m_o;
public:
    token_print_visitor(std::ostream& o)
        : m_o(o)
    {
    }

//public:
    void operator()(const empty&  /*unused*/) const
    {
        m_o << "<empty>";
    }

    void operator()(const quote_sign_t&  /*unused*/) const
    {
        m_o << "'";
    }

    void operator() (bool b) const
    {
        m_o << (b ? "true" : "false");
    }
    void operator()(int64_t i) const
    {
        m_o << i ;
    }
    void operator()(const std::string& s) const
    {
        m_o << sss::raw_string(s);
    }
    void operator()(double d) const
    {
        m_o << d;
    }
    void operator()(const symbol& s ) const
    {
        m_o << s.name();
    }
    void operator() (parenthese_t p) const
    {
        m_o << char(p);
    }
    void operator() (const varlisp::regex_t& r) const
    {
        // NOTE FIXME
        // 这里，应该对串中的空格，以及'/'进行转义后打印
        // escaper()
        if (!r) {
            SSS_POSITION_THROW(std::runtime_error,
                               "nullptr regex-obj");
        }
        m_o << '/' << r->pattern() << '/';
    }
    void operator() (const varlisp::keywords_t& k) const
    {
        m_o << k;
    }
};

inline std::ostream& operator << (std::ostream& o, const Token& tok)
{
    token_print_visitor tv(o);
    boost::apply_visitor(tv, tok);
    return o;
}

class Tokenizer
{
    using StrIterator = std::string::const_iterator;
public:
    Tokenizer();

    explicit Tokenizer(const std::string& data);

    ~Tokenizer() = default;

//public:
    Tokenizer(Tokenizer&& ) = default;
    Tokenizer& operator = (Tokenizer&& ) = default;

//public:
    Tokenizer(const Tokenizer& ) = default;
    Tokenizer& operator = (const Tokenizer& ) = default;

//public:
    Token   top();
    bool    empty() const;

    Token   lookahead_nothrow(int index = 0);
    Token   lookahead(int index = 0);

    bool    consume();

    bool    consume(parenthese_t paren);
    bool    consume(varlisp::keywords_t::kw_type_t type);
    bool    consume(quote_sign_t );

    bool    consume(const symbol& symbol);

    int     consume_count() const
    {
        return !this->m_consumed.empty() ? this->m_consumed.back() : 0;
    }

//public:
    void    append(const std::string& scripts);

    size_t  tokens_size() const;
    bool    is_eof() const;

    void    clear();

    int     retrieve_symbols(std::vector<std::string>& symbols, const char * prefix) const;

    void    print(std::ostream& o) const;
    void    print_token_stack(std::ostream& o) const;

//public:
    void    push(const std::string& data = "");
    void    pop();

protected:
    void    init(const std::string& data);
    bool    parse();

private:
    std::vector<int>            m_consumed;
    std::vector<std::string>    m_data;
    std::vector<StrIterator>    m_beg;
    std::vector<StrIterator>    m_end;

    Token               tok;
    std::string         str_stack;

    //public:
    ss1x::parser::rule  Comment_p;
    ss1x::parser::rule  CommentMulty_p;
    ss1x::parser::rule  Spaces_p;
    ss1x::parser::rule  Quote_p; // '注意，它可以紧贴其他符号使用！
    ss1x::parser::rule  TokenEnd_p;
    ss1x::parser::rule  Decimal_p;
    ss1x::parser::rule  Binary_p;
    ss1x::parser::rule  Hex_p;
    ss1x::parser::rule  Double_p;
    ss1x::parser::rule  Symbol_p;

    ss1x::parser::rule  c_str_escapseq_p;   // '\a', '\r', '\n' ...
    ss1x::parser::rule  String_p;
    ss1x::parser::rule  RawString_p;
    ss1x::parser::rule  Parenthese_p;

    ss1x::parser::rule  BoolTrue_p;
    ss1x::parser::rule  BoolFalse_p;
    ss1x::parser::rule  Token_p;
    ss1x::parser::rule  Regex_p;
    ss1x::parser::rule  FallthrowError_p; // 当其他条件都匹配失败的时候，匹配这个，并消耗非空字符；

    // private:
    std::vector<std::vector<Token> >  m_tokens;

    // int                 m_swap_consumed;
    // std::string         m_swap_data;
    // StrIterator         m_swap_beg;
    // StrIterator         m_swap_end;
    // std::vector<Token>  m_swap_tokens;
};

} // namespace varlisp


#endif /* __TOKENIZER_HPP_1457185659__ */
