#include "tokenizer.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/util/StringSlice.hpp>

#include <ss1x/parser/oparser.hpp>
#include <ss1x/parser/util.hpp>

namespace varlisp {
    Tokenizer::Tokenizer()
        : m_consumed(0)
    {
    }

    Tokenizer::Tokenizer(const std::string& data)
        : m_consumed(0), m_data(data), m_beg(m_data.begin()), m_end(m_data.end())
    {
    }

    Token   Tokenizer::top()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        return this->lookahead_nothrow(0);
    }

    bool    Tokenizer::empty() const
    {
        return this->m_tokens.empty();
    }

    /**
     * @brief 
     *
     * 负数的index没有意义；
     *
     * @param index
     *
     * @return 
     */
    Token   Tokenizer::lookahead_nothrow(int index)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        if (index < 0) {
            return Token();
        }

        while (this->m_beg < this->m_end && int(this->m_tokens.size()) < index + 1) {
            this->parse();
        }
        if (int(this->m_tokens.size()) < index + 1) {
            return Token();
        }
        else {
            return this->m_tokens[index];
        }
    }

    Token   Tokenizer::lookahead(int index)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        Token ret = lookahead_nothrow(index);
        if (!ret.which()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "not enough tokens");
        }
        return ret;
    }

    bool    Tokenizer::consume()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        if (!this->empty()) {
            this->m_tokens.erase(this->m_tokens.begin());
            this->m_consumed++;
            return true;
        }
        return false;
    }

    // NOTE
    // 貌似应该直接扔异常……
    bool    Tokenizer::consume(parenthese_t paren)
    {
        Token target(paren);
        Token current_top = this->top();
        if (current_top == target) {
            this->consume();
            return true;
        }
        return false;
    }

    bool    Tokenizer::consume(const symbol& symbol)
    {
        Token target(symbol);
        Token current_top = this->top();
        if (current_top == target) {
            this->consume();
            return true;
        }
        return false;
    }

    void    Tokenizer::append(const std::string& scripts)
    {
        if (scripts.empty()) {
            return;
        }
        if (!this->m_data.empty()) {
            size_t offset = std::distance(this->m_data.cbegin(), m_beg);

            if (offset) {
                this->m_data.erase(0, offset);
            }
        }

        this->m_data.append("\n");
        // NOTE 这里的附加，从处理逻辑上来说，是一行一行地解析文本；
        // 所以；额外增加一个换行符即可；
        this->m_data.append(scripts);
        m_beg = this->m_data.cbegin();
        m_end = this->m_data.cend();
    }

    size_t  Tokenizer::tokens_size() const
    {
        return this->m_tokens.size();
    }

    bool    Tokenizer::is_eof() const
    {
        return this->m_beg >= this->m_end;
    }

    /**
     * @brief 从字符串中，解析一个Token，并放入序列；
     */
    void    Tokenizer::parse()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, sss::util::make_slice(this->m_beg, this->m_end));
        using namespace ss1x::parser;
        Token tok;

        // NOTE 不同元素(Token)之间，必须要有空白符，或者括号，作为间隔！

        while (m_beg < m_end && std::isspace(*m_beg)) {
            m_beg++;
        }

        rule Comment_p
            = ss1x::parser::char_p(';') >> *(ss1x::parser::char_ - ss1x::parser::char_p('\n') - ss1x::parser::sequence("\r\n"))
            >> &(ss1x::parser::char_p('\n') | ss1x::parser::sequence("\r\n") | ss1x::parser::eof_p);

        rule Spaces_p
            = +ss1x::parser::space_p;

        // rule Expression_p;
        rule TokenEnd_p
            = ss1x::parser::space_p || ss1x::parser::char_set_p("()") || ss1x::parser::eof_p;

        rule Integer_p = (+digit_p)[ss1x::parser::rule::ActionT([&tok](StrIterator it_beg,
                                                                       StrIterator it_end,
                                                                       ss1x::parser::rule::matched_value_t) {
                tok = int(ss1x::parser::util::parseUint32_t(it_beg, it_end));
            })];

        rule Double_p = (double_p > &TokenEnd_p)[ss1x::parser::rule::ActionT([&tok](StrIterator beg,
                                                                                    StrIterator end,
                                                                                    ss1x::parser::rule::matched_value_t v) {
                tok = ss1x::parser::rule::toDouble(v);
;
            })].result(std::function<double(StrIterator,StrIterator)>(&util::slice2double));


        rule Symbol_p
            = (+(ss1x::parser::punct_p - ss1x::parser::char_set_p("#()\""))
            | ss1x::parser::alpha_p >> *(ss1x::parser::alnum_p || ss1x::parser::char_p('-'))
            > &TokenEnd_p)[ss1x::parser::rule::ActionT([&tok](StrIterator beg,
                                                              StrIterator end,
                                                              ss1x::parser::rule::matched_value_t v) {
                tok = symbol(ss1x::parser::rule::toString(v));
;
            })].result(ss1x::parser::util::slice2string);

        rule c_str_escapseq_p =
            ( ss1x::parser::char_p('\\') >>
             (   ss1x::parser::char_set_p("\\abfnrtv'\"")
              | (ss1x::parser::char_range_p('0', '3') >> ss1x::parser::char_range_p('0', '7') >> ss1x::parser::char_range_p('0', '7'))
              | (ss1x::parser::char_range_p('0', '7') || ss1x::parser::char_range_p('0', '7'))
              | (ss1x::parser::char_p('x') > (ss1x::parser::xdigit_p || ss1x::parser::xdigit_p))
             )
            );

        rule String_p
            = ((ss1x::parser::char_p('"')
             >> *( ss1x::parser::char_ - ss1x::parser::char_p('"') - ss1x::parser::char_p('\\')
                  | ss1x::parser::refer(c_str_escapseq_p)
                  | (ss1x::parser::char_p('\\') >> ss1x::parser::char_)) > ss1x::parser::char_p('"')
            )
            > &TokenEnd_p)[ss1x::parser::rule::ActionT([&tok](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                    std::string s = ss1x::parser::rule::toString(v);
                    // TODO 转义等等问题
                tok = s.substr(1, s.length() - 2);
;
            })].result(ss1x::parser::util::slice2string);

        rule LeftParen_p
            = char_p('(')[ss1x::parser::rule::ActionT([&tok](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = varlisp::left_parenthese;
;
            })];

        rule RightParent_p
            = char_p(')')[ss1x::parser::rule::ActionT([&tok](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = varlisp::right_parenthese;
;
            })];

        rule BoolTure_p
            = (ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Tt") > !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
            [ss1x::parser::rule::ActionT([&tok](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = true;
            })];

        rule BoolFalse_p
            = (ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Ff") > !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
            [ss1x::parser::rule::ActionT([&tok](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = false;
            })];

        rule Token_p
            = (  refer(Spaces_p)
               | refer(Comment_p)
               | refer(Double_p)
               | refer(Integer_p)
               | refer(Symbol_p)
               | refer(String_p)
               | refer(LeftParen_p)
               | refer(RightParent_p)
               | refer(BoolTure_p)
               | refer(BoolFalse_p));

        if (Token_p.match(this->m_beg, this->m_end)) {
            SSS_LOG_EXPRESSION(sss::log::log_DEBUG, tok);
            this->m_tokens.push_back(tok);
        }
    }

    void    Tokenizer::clear()
    {
        this->m_data.clear();
        this->m_beg = this->m_data.begin();
        this->m_end = this->m_data.end();
        this->m_tokens.clear();
    }
} // namespace varlisp
