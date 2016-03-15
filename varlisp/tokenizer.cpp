#include "tokenizer.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/util/StringSlice.hpp>

#include <ss1x/parser/oparser.hpp>
#include <ss1x/parser/util.hpp>

namespace varlisp {
    Tokenizer::Tokenizer()
        : m_consumed(0), m_swap_consumed(0)
    {
        this->init();
    }

    Tokenizer::Tokenizer(const std::string& data)
        : m_consumed(0), m_data(data), m_beg(m_data.begin()), m_end(m_data.end()), m_swap_consumed(0)
    {
        this->init();
    }

    void    Tokenizer::init()
    {
        using namespace ss1x::parser;
        this->Comment_p
            = ss1x::parser::char_p(';') >> *(ss1x::parser::char_ - ss1x::parser::char_p('\n') - ss1x::parser::sequence("\r\n"))
            >> &(ss1x::parser::char_p('\n') | ss1x::parser::sequence("\r\n") | ss1x::parser::eof_p);

        this->Spaces_p
            = +ss1x::parser::space_p;

        // rule Expression_p;
        this->TokenEnd_p
            = ss1x::parser::space_p || ss1x::parser::char_set_p("()") || ss1x::parser::eof_p;

        this->Integer_p
            = (+digit_p)[ss1x::parser::rule::ActionT([&](StrIterator it_beg,
                                                         StrIterator it_end,
                                                         ss1x::parser::rule::matched_value_t) {
                tok = int(ss1x::parser::util::parseUint32_t(it_beg, it_end));
            })];

        this->Double_p
            = (double_p > &TokenEnd_p)[ss1x::parser::rule::ActionT([&](StrIterator beg,
                                                                       StrIterator end,
                                                                       ss1x::parser::rule::matched_value_t v) {
                tok = ss1x::parser::rule::toDouble(v);
                ;
            })].result(std::function<double(StrIterator,StrIterator)>(&util::slice2double));


        this->Symbol_p
            = (+(ss1x::parser::punct_p - ss1x::parser::char_set_p("#()\"'"))
               | ss1x::parser::alpha_p >> *(ss1x::parser::alnum_p || ss1x::parser::char_p('-'))
               > &TokenEnd_p)[ss1x::parser::rule::ActionT([&](StrIterator beg,
                                                              StrIterator end,
                                                              ss1x::parser::rule::matched_value_t v) {
                   tok = symbol(ss1x::parser::rule::toString(v));
               })].result(ss1x::parser::util::slice2string);

        this->c_str_escapseq_p =
            ( ss1x::parser::char_p('\\') >>
             (   ss1x::parser::char_set_p("\\abfnrtv'\"")[ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator,rule::matched_value_t){
                 switch (*beg) {
                 case '\\':
                     str_stack.push_back(*beg);
                     break;

                 case 'a':
                     str_stack.push_back('\a');
                     break;
                 case 'b':
                     str_stack.push_back('\b');
                     break;
                 case 'f':
                     str_stack.push_back('\b');
                     break;
                 case 'n':
                     str_stack.push_back('\n');
                     break;
                 case 'r':
                     str_stack.push_back('\r');
                     break;
                 case 't':
                     str_stack.push_back('\t');
                     break;
                 case 'v':
                     str_stack.push_back('\v');
                     break;
                 case '\'':
                     str_stack.push_back('\'');
                     break;
                 case '"':
                     str_stack.push_back('"');
                     break;
                 }
            })]
              | (ss1x::parser::char_range_p('0', '3') >> ss1x::parser::char_range_p('0', '7') >> ss1x::parser::char_range_p('0', '7'))
                  [ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator end,rule::matched_value_t){
                      uint8_t ch = 0u;
                      while (beg != end) {
                          ch *= 8;
                          ch += *beg - '0';
                          ++beg;
                      }
                      str_stack.push_back(ch);
                  })]
              | (ss1x::parser::char_range_p('0', '7') || ss1x::parser::char_range_p('0', '7'))
                  [ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator end,rule::matched_value_t){
                      uint8_t ch = 0u;
                      while (beg != end) {
                          ch *= 8;
                          ch += *beg - '0';
                          ++beg;
                      }
                      str_stack.push_back(ch);
                  })]
              | (ss1x::parser::char_p('x') > (ss1x::parser::xdigit_p || ss1x::parser::xdigit_p))
                  [ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator end,rule::matched_value_t){
                      uint8_t ch = 0u;
                      ++beg;
                      while (beg != end) {
                          ch *= 16;
                          ch += std::isdigit(*beg) ? (*beg - '0') : ((*beg % 16) + 9);
                          ++beg;
                      }
                      str_stack.push_back(ch);
                  })]
             )
            );

        this->String_p
            = ((ss1x::parser::char_p('"')[ss1x::parser::rule::ActionT([&](StrIterator,StrIterator,rule::matched_value_t){
                str_stack.resize(0);
            })]
                >> *( (ss1x::parser::char_ - ss1x::parser::char_p('"') - ss1x::parser::char_p('\\'))
                     [ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator,rule::matched_value_t){
                         str_stack.push_back(*beg);
                     })]
                     | ss1x::parser::refer(c_str_escapseq_p)
                     | (ss1x::parser::char_p('\\') >> ss1x::parser::char_[ss1x::parser::rule::ActionT([&](StrIterator beg,StrIterator,rule::matched_value_t){
                         str_stack.push_back(*beg);
                     })])
                     ) > ss1x::parser::char_p('"')
               )
               > &TokenEnd_p)[ss1x::parser::rule::ActionT([&](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t) {
                // std::string s = ss1x::parser::rule::toString(v);
                // TODO 转义等等问题
                tok = str_stack;
                // tok = s.substr(1, s.length() - 2);
            })].result(ss1x::parser::util::slice2string);

        // RawString_p 可以考虑，用'"开头和"'结尾；因为就算是标准lisp中，也'也只是用作'()的标记；
        this->RawString_p
            = (sequence("'\"(") >> *(char_ - sequence(")\"'")) > sequence(")\"'"))
            [ss1x::parser::rule::ActionT([&](StrIterator beg, StrIterator end, ss1x::parser::rule::matched_value_t) {
                auto s = sss::util::make_slice(beg, end);
                s.shrink(3, 3);
                tok = s.str();
            })];

        this->LeftParen_p
            = char_p('(')[ss1x::parser::rule::ActionT([&](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = varlisp::left_parenthese;
                ;
            })];

        this->RightParent_p
            = char_p(')')[ss1x::parser::rule::ActionT([&](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = varlisp::right_parenthese;
                ;
            })];

        this->BoolTure_p
            = (ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Tt") > !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
            [ss1x::parser::rule::ActionT([&](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = true;
            })];

        this->BoolFalse_p
            = (ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Ff") > !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
            [ss1x::parser::rule::ActionT([&](StrIterator, StrIterator, ss1x::parser::rule::matched_value_t v) {
                tok = false;
            })];

        this->Token_p
            = (  refer(Spaces_p)
               | refer(Comment_p)
               | refer(Double_p)
               | refer(Integer_p)
               | refer(Symbol_p)
               | refer(String_p)
               | refer(RawString_p)
               | refer(LeftParen_p)
               | refer(RightParent_p)
               | refer(BoolTure_p)
               | refer(BoolFalse_p));
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

    void    Tokenizer::push()
    {
        std::swap(m_consumed, m_swap_consumed);
        std::swap(m_data, m_swap_data);
        std::swap(m_beg, m_swap_beg);
        std::swap(m_end, m_swap_end);
        std::swap(m_tokens, m_swap_tokens);
    }

    void    Tokenizer::pop()
    {
        // TODO FIXME
        // 貌似 clear()动作，没有对m_swap_consumed进行处理！
        this->m_consumed = 0;
        this->clear();
        this->push();
    }

    /**
     * @brief 从字符串中，解析一个Token，并放入序列；
     */
    void    Tokenizer::parse()
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, sss::util::make_slice(this->m_beg, this->m_end));

        // NOTE 不同元素(Token)之间，必须要有空白符，或者括号，作为间隔！

        while (m_beg < m_end && std::isspace(*m_beg)) {
            m_beg++;
        }

        if (Token_p.match(this->m_beg, this->m_end)) {
            SSS_LOG_EXPRESSION(sss::log::log_DEBUG, this->tok);
            this->m_tokens.push_back(this->tok);
        }
    }

    int     Tokenizer::retrieve_symbols(std::vector<std::string>& symbols, const char * prefix) const
    {
        for (const auto& item : this->m_tokens) {
            if (const varlisp::symbol * ps = boost::get<varlisp::symbol>(&item)) {
                if (sss::is_begin_with(ps->m_data, prefix)) {
                    symbols.push_back(ps->m_data);
                }
            }
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
