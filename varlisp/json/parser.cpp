#include "parser.hpp"

#include <cctype>
#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>

#include "../detail/list_iterator.hpp"

namespace varlisp {
namespace json {

struct JParser
{
    JParser(sss::string_view s, Object& ret)
    {
        parse(s, ret);
    }
    bool parse(sss::string_view& s, Object& ret)
    {
        COLOG_DEBUG(s);
        this->skip_white_space(s);
        if (s.empty()) {
            SSS_POSITION_THROW(std::runtime_error, "empty string");
        }
        switch(s.front()) {
            case '{':
                parse_object(s, ret);
                return true;

            case '[':
                parse_array(s, ret);
                return true;

            default:
                COLOG_DEBUG(s);
                return false;
        }
    }

    void parse_value(sss::string_view& s, Object& ret)
    {
        if (parse(s, ret)) {
            return;
        }
        else {
            COLOG_DEBUG(s);
            switch (s.front()) {
                case '"':
                    {
                        std::string str;
                        this->parse_string(s, str);
                        ret = varlisp::string_t(std::move(str));
                    }
                    break;

                case '-':
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    {
                        this->parse_number(s, ret);
                    }
                    break;

                case 't': case 'f':
                    {
                        this->parse_bool(s, ret);
                    }
                    break;

                case 'n':
                    {
                        COLOG_DEBUG(s);
                        this->parse_null(s, ret);
                    }
                    break;

                default:
                    SSS_POSITION_THROW(std::runtime_error,
                                       "unexpect string ", s);
                    break;
            }
        }
    }
    void skip_white_space(sss::string_view& s)
    {
        while(!s.empty() && std::isspace(s.front())) {
            s.pop_front();
        }
    }

    void parse_object(sss::string_view& s, Object& ret)
    {
        this->consume_or_throw(s, '{', "expect '{'");
        varlisp::Environment env;
        bool is_first = true;
        while (!s.empty() && s.front() != '}') {
            if (is_first) {
                is_first = false;
            }
            else {
                this->skip_white_space(s);
                this->consume_or_throw(s, ',', "expect ','");
            }
            std::string key;
            this->skip_white_space(s);
            parse_string(s, key);
            this->skip_white_space(s);
            this->consume_or_throw(s, ':', "expect ':'");
            Object val;
            this->parse_value(s, val);
            env[key] = std::move(val);
        }
        this->consume_or_throw(s, '}', "expect '}'");
        ret = std::move(env);
    }
    void parse_array(sss::string_view& s, Object& ret)
    {
        this->consume_or_throw(s, '[', "expect '['");
        varlisp::List list = varlisp::List::makeSQuoteList();
        auto back_it = varlisp::detail::list_back_inserter<varlisp::Object>(list);
        bool is_first = true;
        while (!s.empty() && s.front() != ']') {
            if (is_first) {
                is_first = false;
            }
            else {
                this->skip_white_space(s);
                this->consume_or_throw(s, ',', "expect ','");
            }
            Object val;
            this->skip_white_space(s);
            COLOG_DEBUG(s.front());
            this->parse_value(s, val);
            *back_it++ = val;
        }
        this->consume_or_throw(s, ']', "expect ']'");
        ret = std::move(list);
    }

    void parse_string(sss::string_view& s, std::string& ret)
    {
        this->consume_or_throw(s, '"', "expect '\"'");
        ret.resize(0);

        while (!s.empty() && s.front() != '"') {
            switch (s.front()) {
            case '\\':
                {
                    if (s.size() < 2) {
                        SSS_POSITION_THROW(std::runtime_error,
                                           "unfinished escape char", s);
                    }
                    const char * it_b2 = s.begin();
                    ++it_b2;
                    switch (*it_b2) {
                    case '"':   // quotation mark
                        ret.push_back('"');
                        s.remove_prefix(2);
                        break;
                    case '\\':  // reverse solidus
                        ret.push_back('\\');
                        s.remove_prefix(2);
                        break;
                    case '/':   // solidus
                        ret.push_back('/');
                        s.remove_prefix(2);
                        break;
                    case 'b':   // backspace
                        ret.push_back('\b');
                        s.remove_prefix(2);
                        break;
                    case 'f':   // formfeed
                        ret.push_back('\f');
                        s.remove_prefix(2);
                        break;
                    case 'n':   // newline
                        ret.push_back('\n');
                        s.remove_prefix(2);
                        break;
                    case 'r':   // carriage return
                        ret.push_back('\r');
                        s.remove_prefix(2);
                        break;

                    case 't':   // horizontal tab
                        ret.push_back('\t');
                        s.remove_prefix(2);
                        break;

                    case 'u':   // 4hexadecimal digits
                        {
                            if (s.size() < 6) {
                                SSS_POSITION_THROW(std::runtime_error,
                                                   "unfinished ascii unicode", s);
                            }
                            const char * it_hex = s.begin() + 2;
                            for (int i = 0; i < 4; ++i, ++it_hex) {
                                if (!std::isxdigit(*it_hex)) {
                                    SSS_POSITION_THROW(std::runtime_error,
                                                       "unfinished ascii unicode", s);
                                }
                            }
                            s.remove_prefix(6);
                        }
                        break;

                    default:
                        SSS_POSITION_THROW(std::runtime_error,
                                           "unkown escape sequence", s);
                    }
                }
                break;

            default:
                if (std::iscntrl(s.front())) {
                    SSS_POSITION_THROW(std::runtime_error,
                                       "unkonw control char", sss::raw_char(s.front()));
                }
                else {
                    ret.push_back(s.front());
                    s.pop_front();
                }
                break;
            }
        }
        this->consume_or_throw(s, '"', "expect '\"'");
    }

    void parse_number(sss::string_view& s, Object& ret)
    {
        sss::string_view s_saved = s;
        bool is_double = false;
        switch (s.front()) {
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            break;

        default:
            SSS_POSITION_THROW(std::runtime_error,
                               "unkonw char", s);
        }
        // 先跳过负号
        if (s.front() == '-') {
            s.pop_front();
            if (s.empty()) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "unfinished -number", s);
            }
        }

        // 整数部分
        if (s.front() == '0' && (s.size() >= 2 && !std::isdigit(s[1]))) {
            s.pop_front();
        }
        else if ('1' <= s.front() && s.front() <= '9') {
            while (!s.empty() && std::isdigit(s.front())) {
                s.pop_front();
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "unfinished number", s);
        }

        // 小数部分
        if (s.size() >= 2 && s[0] == '.' && std::isdigit(s[1]))
        {
            is_double = true;
            s.remove_prefix(2);
            while (!s.empty() && std::isdigit(s.front())) {
                s.pop_front();
            }
        }

        // e指数部分
        if (!s.empty() && std::toupper(s[0]) == 'E') {
            s.pop_front();
            is_double = true;
            this->consume(s, '+') ||this->consume(s, '-');
            int e_cnt = 0;
            while (!s.empty() && std::isdigit(s.front())) {
                s.pop_front();
                e_cnt++;
            }
            if (!e_cnt) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "unfinished e-number", s);
            }
        }

        if (is_double)
        {
            ret = sss::string_cast<double>(std::string(s_saved.begin(), s.begin()));
        }
        else {
            ret = sss::string_cast<int>(std::string(s_saved.begin(), s.begin()));
        }
    }

    void parse_bool(sss::string_view& s, Object& ret)
    {
        if (s.substr(0, 4) == sss::string_view("true")) {
            ret = true;
            s.remove_prefix(4);
        }
        else if (s.substr(0, 5) == sss::string_view("false")) {
            ret = false;
            s.remove_prefix(5);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "parse bool erro: ", s);
        }
    }

    void parse_null(sss::string_view& s, Object& ret)
    {
        if (s.substr(0, 4) == sss::string_view("null")) {
            ret = varlisp::Nill{};
            s.remove_prefix(4);
        }
    }

    bool consume(sss::string_view& s, char c)
    {
        if (!s.empty() && s.front() == c) {
            s.pop_front();
            return true;
        }
        return false;
    }

    template<typename T>
    void consume_or_throw(sss::string_view& s, const T& v, sss::string_view msg)
    {
        if (!this->consume(s, v)) {
            SSS_POSITION_THROW(std::runtime_error, msg.to_string());
        }
    }
};

Object parse(sss::string_view s)
{
    Object ret;
    JParser jp(s, ret);
    return ret;
}
} // namespace json
} // namespace varlisp
