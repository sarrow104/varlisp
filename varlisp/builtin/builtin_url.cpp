#include <vector>
#include <cstring>

#include <re2/re2.h>

#include <sss/spliter.hpp>
#include <sss/path.hpp>
#include <sss/utlstring.hpp>

#include <ss1x/asio/utility.hpp>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../json_print_visitor.hpp"
#include "../json/parser.hpp"
#include "../detail/car.hpp"
#include "../detail/url.hpp"
#include "../raw_stream_visitor.hpp"

namespace varlisp {

namespace detail {
inline bool url_is_reserved_char(char ch)
{
    return std::isalnum(ch) || std::strchr("-_.*", ch);
}
// NOTE 需要处理不可打印，以及空白，还有部分符号。
struct url_encode_t
{
    sss::string_view str;
    explicit url_encode_t(const sss::string_view& s) : str(s) {}
    void print(std::ostream& o) const
    {
        for (size_t i = 0; i< str.size(); ++i) {
            if (detail::url_is_reserved_char(str[i])) {
                o << str[i];
            }
            else {
                o << '%' << sss::lower_hex2char(str[i] >> 4u) << sss::lower_hex2char(str[i]);
            }
        }
    }
};

struct url_decode_t
{
    sss::string_view str;
    explicit url_decode_t(const sss::string_view& s) : str(s) {}
    void print(std::ostream& o) const
    {
        size_t i = 0;
        while (i < str.size()) {
            if (str[i] == '%') {
                if (i + 2 < str.size() && std::isxdigit(str[i + 1]) && std::isxdigit(str[i + 2])) {
                    char ch = (sss::hex2int(str[i + 1]) << 4u) | sss::hex2int(str[i + 2]);
                    o << ch;
                    i += 3;
                }
                else {
                    COLOG_ERROR("unrecongnize percent-sequnce:", sss::raw_string(str.substr(i)));
                    break;
                }
            }
            else if (!detail::url_is_reserved_char(str[i])) {
                COLOG_ERROR("try to decode un-encoded:", sss::raw_char(str[i]));
                break;
            }
            else {
                o << str[i];
                ++i;
            }
        }
    }
};

std::string url_encode(const sss::string_view& str)
{
    std::ostringstream oss;
    url_encode_t(str).print(oss);
    return oss.str();
}

std::string url_decode(const sss::string_view& str)
{
    std::ostringstream oss;
    url_decode_t(str).print(oss);
    return oss.str();
}

} // namespace detail

REGIST_BUILTIN("url-split", 1, 1, eval_url_split,
               "; url-split 拆分url地址\n"
               "(url-split \"url-string\") -> '(protocal domain port path {parameters})");

// TODO NOTE url-split，并没有重用内部的串。应该考虑重用——毕竟，split后，各部
// 分只是原始串的切片。
Object eval_url_split(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-split";
    std::array<Object, 1> objs;
    auto * p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    auto parts = ss1x::util::url::split_port_auto(*p_url->gen_shared());
    auto parts_list = varlisp::List();
    parts_list.append(string_t(std::move(std::get<0>(parts))));
    parts_list.append(string_t(std::move(std::get<1>(parts))));
    parts_list.append(int64_t(std::get<2>(parts)));
    auto q_pos = std::get<3>(parts).find('?');
    std::string parameters;
    if (q_pos == std::string::npos) {
        parts_list.append(string_t(std::move(std::get<3>(parts))));
    }
    else {
        parts_list.append(string_t(std::move(std::get<3>(parts).substr(0, q_pos))));
        parameters = std::get<3>(parts).substr(q_pos + 1);
        varlisp::Environment param_env;
        if (!parameters.empty()) {
            sss::ViewSpliter<char> sp(parameters, '&');
            sss::string_view item;
            while (sp.fetch(item)) {
                auto eq_pos = item.find('=');
                if (eq_pos != sss::string_view::npos) {
                    // NOTE TODO FIXME
                    // 1. 名字部分，需要符合lisp的symbol
                    // 2. escape-sequence，应该如何处理？
                    param_env[detail::url_decode(item.substr(0, eq_pos))] = string_t(detail::url_decode(item.substr(eq_pos + 1)));
                }
                else {
                    param_env[detail::url_decode(item)] = string_t(std::move(std::string()));
                }
                COLOG_DEBUG(item);
            }
        }
        parts_list.append(param_env);
    }
    return varlisp::List::makeSQuoteObj(parts_list);
}

REGIST_BUILTIN("url-join", 1, 1, eval_url_join,
               "; url-join 合并url地址\n"
               "(url-join '(protocal domain port path {parameters})) -> \"url-string\"");

Object eval_url_join(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-join";
    std::array<Object, 1> objs;
    const auto * p_list = getQuotedList(env, args.nth(0), objs[0]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_list, funcName, 1, DEBUG_INFO);
    std::string protocal;
    std::string domain;
    int port = 80;
    std::string path;
    {
        Object tmp;
        for (size_t i = 0; i < p_list->length(); ++i) {
            switch (i)
            {
                case 0: protocal = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 1: domain   = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 2: port     = *requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO); break;
                case 3: path     = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 4:
                        {
                            const auto * p_params =
                                requireTypedValue<varlisp::Environment>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO);
                            bool is_1st = true;
                            for (auto it = p_params->begin(); it != p_params->end(); ++it) {
                                if (is_1st) {
                                    path += '?';
                                    is_1st = false;
                                }
                                else {
                                    path += '&';
                                }
                                path += it->first;
                                std::ostringstream oss;
                                boost::apply_visitor(raw_stream_visitor(oss, env), it->second.first);
                                std::string value = oss.str();
                                if (!value.empty()) {
                                    path += '=';
                                    path += detail::url_encode(oss.str());
                                }
                            }
                        }
                        break;

            }
        }
    }

    return string_t(std::move(ss1x::util::url::join(protocal, domain, port, path)));
}

REGIST_BUILTIN("url-full", 2, 2, eval_url_full,
               "; url-full 根据base-url或者domain字符串，补全url地址\n"
               "(url-full target-string mapping-url) -> \"full-url-string\"");

Object eval_url_full(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-full";
    std::array<Object, 2> objs;
    auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    auto * p_maping_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);  

    auto target = p_target_string->to_string();
    bool is_modified = varlisp::detail::url::full_of(target, *p_maping_url->gen_shared());
    if (is_modified) {
        return string_t(std::move(target));
    }
    else {
        return *p_target_string;
    }
}

REGIST_BUILTIN("url-encode", 1, 1, eval_url_encode,
               "; url-encode url地址编码。详细说明，见：\n"
               "; http://en.wikipedia.org/wiki/Percent-encoding\n"
               "; urlEncde 又名 percent-encoding；即，以百分号'%'为转移引导字符。\n"
               "; 当前，保留(不用转移)的字符有：\n"
               "; !*'();:@&=+$,/?#[]\n"
               "; 以及26个大小写字母，还有10个数字字符\n"
               "(url-encode \"string\") -> \"encoded-string\"");

Object eval_url_encode(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-encode";
    std::array<Object, 1> objs;
    auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    return string_t(detail::url_encode(p_target_string->to_string_view()));
}

REGIST_BUILTIN("url-decode", 1, 1, eval_url_decode,
               "; url-decode 对url地址进行解码。详细说明，见：\n"
               "; http://en.wikipedia.org/wiki/Percent-encoding\n"
               "(url-decode \"encoded-string\") -> \"string\"");

Object eval_url_decode(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-decode";
    std::array<Object, 1> objs;
    auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    return string_t(detail::url_decode(p_target_string->to_string_view()));
}

} // namespace varlisp
