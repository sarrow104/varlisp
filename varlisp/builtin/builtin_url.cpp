#include <vector>
#include <cstring>

#include <re2/re2.h>

#include <sss/spliter.hpp>
#include <sss/path.hpp>

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

REGIST_BUILTIN("url-split", 1, 1, eval_url_split,
               "; url-split 拆分url地址\n"
               "(url-split \"url-string\") -> '(protocal domain port path {parameters})");

Object eval_url_split(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-split";
    std::array<Object, 1> objs;
    auto * p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    auto parts = ss1x::util::url::split_port_auto(p_url->to_string());
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
                    param_env[item.substr(0, eq_pos).to_string()] = string_t(std::move(item.substr(eq_pos + 1).to_string()));
                }
                else {
                    param_env[item.to_string()] = string_t(std::move(std::string()));
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
                case 0: protocal = requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->to_string(); break;
                case 1: domain = requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->to_string(); break;
                case 2: port = *requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO); break;
                case 3: path = requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->to_string(); break;
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
                                path += '=';
                                path += oss.str();
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
    bool is_modified = varlisp::detail::url::full_of(target, p_maping_url->to_string());
    if (is_modified) {
        return string_t(std::move(target));
    }
    else {
        return *p_target_string;
    }
}

} // namespace varlisp
