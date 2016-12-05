#include "tokenizer.hpp"

#include <stdexcept>

#include <sss/path.hpp>
#include <sss/util/Parser.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sss/util/StringSlice.hpp>
#include <sss/colorlog.hpp>

#ifndef DEBUG
#define SSS_COLOG_TURNOFF
#endif

#include <sss/colorlog.hpp>

#include <ss1x/parser/oparser.hpp>
#include <ss1x/parser/util.hpp>

namespace varlisp {
Tokenizer::Tokenizer() { this->init(""); }
Tokenizer::Tokenizer(const std::string& data) { this->init(data); }
void Tokenizer::init(const std::string& data)
{
    using namespace ss1x::parser;
    this->Comment_p =
        (ss1x::parser::char_p(';') >>
         *(ss1x::parser::char_ - ss1x::parser::char_p('\n') -
           ss1x::parser::sequence("\r\n")) >>
         &(ss1x::parser::char_p('\n') | ss1x::parser::sequence("\r\n") |
           ss1x::parser::eof_p))
            .name("Comment_p")[ss1x::parser::rule::ActionT([&](
                StrIterator, StrIterator, ss1x::parser::rule::matched_value_t) {
                tok = varlisp::empty();
            })];

    this->Spaces_p =
        (+ss1x::parser::space_p)
            .name("Spaces_p")[ss1x::parser::rule::ActionT([&](
                StrIterator, StrIterator, ss1x::parser::rule::matched_value_t) {
                tok = varlisp::empty();
            })];

    this->TokenEnd_p = (ss1x::parser::space_p ||
                        ss1x::parser::char_set_p("()[]{}") || ss1x::parser::eof_p)
                           .name("TokenEnd_p");

    this->Decimal_p =
        ((+digit_p >> &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
             StrIterator it_beg, StrIterator it_end,
             ss1x::parser::rule::matched_value_t) {
            tok = int64_t(ss1x::parser::util::parseUint64_t(it_beg, it_end));
        })]).name("Decimal_p");

    this->Hex_p =
        (((char_p('0') >> char_set_p("xX") > +xdigit_p) >> &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
             StrIterator it_beg, StrIterator it_end,
             ss1x::parser::rule::matched_value_t) {
            int64_t h = 0;
            for (StrIterator it = std::next(it_beg, 2); it != it_end; ++it) {
                h <<= 4;
                h += sss::util::Parser<StrIterator>::hexchar2number(*it);
            }
            tok = h;
        })]).name("Hex_p");

    this->Double_p =
        ((double_p > &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
                                      StrIterator beg, StrIterator end,
                                      ss1x::parser::rule::matched_value_t v) {
             tok = ss1x::parser::rule::toDouble(v);
             ;
         })].result(std::function<double(StrIterator,
                                         StrIterator)>(&util::slice2double)))
            .name("Double_p");

    this->Symbol_p =
        ((+(ss1x::parser::punct_p - ss1x::parser::char_set_p("#()[]{}\"'_")) |
          (ss1x::parser::utf8_range_p("一", "龥") | ss1x::parser::alpha_p) >>
              *((ss1x::parser::utf8_range_p("一", "龥") | ss1x::parser::alnum_p) || ss1x::parser::char_p('_') ||
                ss1x::parser::char_p('-') || ss1x::parser::char_p('?') ||
                ss1x::parser::char_p(':') || ss1x::parser::char_p('!')) >
              &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
                               StrIterator beg, StrIterator end,
                               ss1x::parser::rule::matched_value_t v) {
             tok = symbol(ss1x::parser::rule::toString(v));
         })].result(ss1x::parser::util::slice2string))
            .name("Symbol_p");

    this->c_str_escapseq_p =
        ((ss1x::parser::char_p('\\') >>
          (ss1x::parser::char_set_p("\\abfnrtv'\"")[ss1x::parser::rule::ActionT(
               [&](StrIterator beg, StrIterator, rule::matched_value_t) {
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
                           str_stack.push_back('\f');
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
               })] |
           (ss1x::parser::char_range_p('0', '3') >>
            ss1x::parser::char_range_p('0', '7') >>
            ss1x::parser::char_range_p('0', '7'))[ss1x::parser::rule::ActionT(
               [&](StrIterator beg, StrIterator end, rule::matched_value_t) {
                   uint8_t ch = 0u;
                   while (beg != end) {
                       ch *= 8;
                       ch += *beg - '0';
                       ++beg;
                   }
                   str_stack.push_back(ch);
               })] |
           (ss1x::parser::char_range_p('0', '7') ||
            ss1x::parser::char_range_p('0', '7'))[ss1x::parser::rule::ActionT(
               [&](StrIterator beg, StrIterator end, rule::matched_value_t) {
                   uint8_t ch = 0u;
                   while (beg != end) {
                       ch *= 8;
                       ch += *beg - '0';
                       ++beg;
                   }
                   str_stack.push_back(ch);
               })] |
           (ss1x::parser::char_p('x') >
            (ss1x::parser::xdigit_p || ss1x::parser::xdigit_p))
               [ss1x::parser::rule::ActionT([&](
                   StrIterator beg, StrIterator end, rule::matched_value_t) {
                   uint8_t ch = 0u;
                   ++beg;
                   while (beg != end) {
                       ch *= 16;
                       ch += std::isdigit(*beg) ? (*beg - '0')
                                                : ((*beg % 16) + 9);
                       ++beg;
                   }
                   str_stack.push_back(ch);
               })])))
            .name("c_str_escapseq_p");

    this->String_p =
        (((ss1x::parser::char_p('"')[ss1x::parser::rule::ActionT(
               [&](StrIterator, StrIterator, rule::matched_value_t) {
                   str_stack.resize(0);
               })] >>
           *((ss1x::parser::char_ - ss1x::parser::char_p('"') -
              ss1x::parser::char_p('\\'))[ss1x::parser::rule::ActionT(
                 [&](StrIterator beg, StrIterator end, rule::matched_value_t) {
                     std::copy(beg, end, std::back_inserter(str_stack));
                 })] |
             ss1x::parser::refer(c_str_escapseq_p) |
             (ss1x::parser::char_p('\\') >>
              ss1x::parser::char_[ss1x::parser::rule::ActionT(
                  [&](StrIterator beg, StrIterator end, rule::matched_value_t) {
                      std::copy(beg, end, std::back_inserter(str_stack));
                  })])) > ss1x::parser::char_p('"')) >
          &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
                           StrIterator, StrIterator,
                           ss1x::parser::rule::matched_value_t) {
             tok = str_stack;
         })].result(ss1x::parser::util::slice2string))
            .name("String_p");

    // RawString_p
    // 可以考虑，用'"开头和"'结尾；因为就算是标准lisp中，也'也只是用作'()的标记；
    this->RawString_p = ((sequence("'\"(") >> *(char_ - sequence(")\"'")) >
                          sequence(")\"'"))[ss1x::parser::rule::ActionT([&](
                             StrIterator beg, StrIterator end,
                             ss1x::parser::rule::matched_value_t) {
                            auto s = sss::util::make_slice(beg, end);
                            s.shrink(3, 3);
                            tok = s.str();
                        })]).name("RawString_p");

    this->Parenthese_p = (char_set_p("()[]{}")[ss1x::parser::rule::ActionT([&](
             StrIterator beg, StrIterator, ss1x::parser::rule::matched_value_t v) {
            tok = varlisp::parenthese_t(*beg);
        })]).name("Parenthese_p");

    this->BoolTrue_p =
        ((ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Tt") >
          !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
             [ss1x::parser::rule::ActionT(
                 [&](StrIterator, StrIterator,
                     ss1x::parser::rule::matched_value_t v) { tok = true; })])
            .name("BoolTrue_p");

    this->BoolFalse_p =
        ((ss1x::parser::char_p('#') >> ss1x::parser::char_set_p("Ff") >
          !(ss1x::parser::alnum_p | ss1x::parser::char_p('-')))
             [ss1x::parser::rule::ActionT(
                 [&](StrIterator, StrIterator,
                     ss1x::parser::rule::matched_value_t v) { tok = false; })])
            .name("BoolFalse_p");

    this->Regex_p =
        ((ss1x::parser::char_p('/') >> *(sequence("\\ ") | sequence("\\/") |
                                         (char_ - char_p(' ') - char_p('/'))) >
          ss1x::parser::char_p('/') >
          &TokenEnd_p)[ss1x::parser::rule::ActionT([&](
             StrIterator beg, StrIterator end, ss1x::parser::rule::matched_value_t v) {
            tok = sss::regex::CRegex(std::string(beg + 1, end - 1));
        })]).name("Regex_p");

    this->FallthrowError_p =
        (+(ss1x::parser::char_ -
           ss1x::parser::space_p)[ss1x::parser::rule::ActionT([&](
             StrIterator beg, StrIterator end,
             ss1x::parser::rule::matched_value_t) {

               COLOG_ERROR(m_tokens, sss::raw_string(std::string(beg, end)));
            SSS_POSITION_THROW(std::runtime_error, "Un-recongnise string `",
                              std::string(beg, end), "`");
        })]).name("FallthrowError_p");

    this->Token_p =
        (refer(Spaces_p) | refer(Comment_p) |
         refer(Hex_p) |
         refer(Decimal_p) |
         // 将double放在int后面，只是为了验证解析器不会丢失匹配；我的Integer_p不会
         // 遗漏'.'等字符。
         // refer(Integer_p) |
         refer(Double_p) |
         refer(Regex_p) |
         refer(Parenthese_p) |
         refer(String_p) |
         refer(RawString_p) |
         refer(Symbol_p) |
         refer(BoolTrue_p) | refer(BoolFalse_p) | refer(FallthrowError_p));

    this->push(data);
}

Token Tokenizer::top()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    return this->lookahead_nothrow(0);
}

bool Tokenizer::empty() const
{
    return this->m_tokens.empty() || this->m_tokens.back().empty();
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
Token Tokenizer::lookahead_nothrow(int index)
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    if (index < 0) {
        return Token();
    }

    while (this->m_beg < this->m_end &&
           int(this->m_tokens.back().size()) < index + 1) {
        if (!this->parse()) {
            break;
        }
    }
    if (int(this->m_tokens.back().size()) < index + 1) {
        return Token();
    }
    else {
        return this->m_tokens.back()[index];
    }
}

Token Tokenizer::lookahead(int index)
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    Token ret = lookahead_nothrow(index);
    if (!ret.which()) {
        SSS_POSITION_THROW(std::runtime_error, "not enough tokens");
    }
    return ret;
}

bool Tokenizer::consume()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    if (!this->empty()) {
        COLOG_DEBUG('(', this->m_tokens.back().front(), "); left = ",
                    sss::raw_string(
                        std::string(this->m_beg.back(), this->m_end.back())));
        this->m_tokens.back().erase(this->m_tokens.back().begin());
        this->m_consumed.back()++;
        return true;
    }
    return false;
}

// NOTE
// 貌似应该直接扔异常……
bool Tokenizer::consume(parenthese_t paren)
{
    Token target(paren);
    Token current_top = this->top();
    if (current_top == target) {
        this->consume();
        return true;
    }
    return false;
}

bool Tokenizer::consume(const symbol& symbol)
{
    Token target(symbol);
    Token current_top = this->top();
    if (current_top == target) {
        this->consume();
        return true;
    }
    return false;
}

void Tokenizer::append(const std::string& scripts)
{
    if (scripts.empty()) {
        return;
    }
    if (!this->m_data.back().empty()) {
        size_t offset =
            std::distance(this->m_data.back().cbegin(), m_beg.back());

        if (offset) {
            COLOG_DEBUG("erase(",
                        sss::raw_string(this->m_data.back().substr(0, offset)),
                        ");");
            this->m_data.back().erase(0, offset);
            COLOG_DEBUG("left(", sss::raw_string(this->m_data.back()), "); @",
                        &this->m_data.back());
        }
    }

    this->m_data.back().append("\n");
    // NOTE 这里的附加，从处理逻辑上来说，是一行一行地解析文本；
    // 所以；额外增加一个换行符即可；
    this->m_data.back().append(scripts);
    m_beg.back() = this->m_data.back().cbegin();
    m_end.back() = this->m_data.back().cend();
    COLOG_DEBUG("distance = ", std::distance(m_beg.back(), m_end.back()));
}

size_t Tokenizer::tokens_size() const { return this->m_tokens.back().size(); }
bool Tokenizer::is_eof() const
{
    return this->m_beg.empty() || this->m_beg.back() >= this->m_end.back();
}

void Tokenizer::push(const std::string& data)
{
    COLOG_DEBUG('(', data, ')');
    if (!this->m_consumed.empty()) {
        COLOG_DEBUG("left = ", sss::raw_string(std::string(
                                   this->m_beg.back(), this->m_end.back())));
    }
    // this->print_token_stack(std::cout);

    this->m_consumed.push_back(0);
    this->m_data.push_back(data);
    this->m_beg.push_back(this->m_data.back().begin());
    this->m_end.push_back(this->m_data.back().end());
    this->m_tokens.push_back(std::vector<Token>());
}

void Tokenizer::pop()
{
    COLOG_DEBUG("");
    this->m_consumed.pop_back();
    this->m_data.pop_back();
    this->m_beg.pop_back();
    this->m_end.pop_back();
    this->m_tokens.pop_back();
}

/**
 * @brief 从字符串中，解析一个Token，并放入序列；
 */
bool Tokenizer::parse()
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    SSS_LOG_EXPRESSION(
        sss::log::log_DEBUG,
        sss::util::make_slice(this->m_beg.back(), this->m_end.back()));

    // NOTE 不同元素(Token)之间，必须要有空白符，或者括号，作为间隔！

    while (m_beg.back() < m_end.back() && std::isspace(*m_beg.back())) {
        m_beg.back()++;
    }

    auto beg_sv = this->m_beg.back();
    if (Token_p.match(this->m_beg.back(), this->m_end.back()) &&
        this->tok.which()) {
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, this->tok);
        COLOG_DEBUG(".push_back(", this->tok, ") from ",
                    sss::raw_string(std::string(beg_sv, this->m_beg.back())));
        this->m_tokens.back().push_back(this->tok);
        this->tok = varlisp::empty();
    }
    return std::distance(beg_sv, this->m_beg.back());
}

int Tokenizer::retrieve_symbols(std::vector<std::string>& symbols,
                                const char* prefix) const
{
    for (const auto& item : this->m_tokens.back()) {
        if (const varlisp::symbol* ps = boost::get<varlisp::symbol>(&item)) {
            if (sss::is_begin_with(ps->m_data, prefix)) {
                symbols.push_back(ps->m_data);
            }
        }
    }
}

void Tokenizer::clear()
{
    this->m_data.back().clear();
    this->m_beg.back() = this->m_data.back().begin();
    this->m_end.back() = this->m_data.back().end();
    this->m_tokens.back().clear();
}

void Tokenizer::print(std::ostream& o) const
{
    o << this->Token_p << std::endl;
}

void Tokenizer::print_token_stack(std::ostream& o) const
{
#ifdef DEBUG
    if (!this->m_tokens.empty()) {
        COLOG_DEBUG("m_data = ", this->m_data.back());
        COLOG_DEBUG("offset = ",
                    std::distance(this->m_data.back().cbegin(), m_beg.back()));
        COLOG_DEBUG("length = ", this->m_data.back().length());
        COLOG_DEBUG("(std::ostream&) ", this->m_tokens.size(),
                    this->m_tokens.back().size());

        int i = 0;
        for (const auto& tok : this->m_tokens.back()) {
            COLOG_DEBUG("\t[ ", i++, "] = ", tok);
        }
    }
    else {
        COLOG_DEBUG("(std::ostream&) empty stack");
    }
#endif
}
}  // namespace varlisp
