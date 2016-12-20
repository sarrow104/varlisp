#include <cctype>

#include <array>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <sss/algorithm.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/encoding.hpp>
#include <sss/iConvpp.hpp>
#include <sss/utlstring.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>

#include "../builtin_helper.hpp"
#include "../object.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/http.hpp"
#include "../detail/cookie.hpp"

namespace varlisp {

REGIST_BUILTIN(
    "http-get", 1, 3, eval_http_get,
    "(http-get \"url\") -> \"<html>\"\n"
    "(http-get \"url\" \"proxy-url\" proxy-port-number) -> \"<html>\"");

/**
 * @brief
 *        (http-get "url") -> "<html>"
 *        (http-get "url" "proxy-url" proxy-port-number) -> "<html>"
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_http_get(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "http-get";
    if (args.length() == 2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": need 1 or 3 parameters)");
    }
    std::array<Object, 3> objs;
    const string_t* p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_proxy = 0;
    const int64_t* p_port = 0;

    if (args.length() == 3) {
        p_proxy = 
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

        p_port = 
            requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);
    }

    ss1x::http::Headers headers;

    std::string max_content;

    std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string& url)> downloadFunc;

    if (detail::CookieMgr_t::get_cookie_enable_status()) {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpGetCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     p_proxy->to_string(), *p_port,
                                     std::placeholders::_3,
                                     varlisp::detail::CookieMgr_t::getCookie);
        }
        else {
            downloadFunc = std::bind(ss1x::asio::redirectHttpGetCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     varlisp::detail::CookieMgr_t::getCookie);
        }
    }
    else {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpGet,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     p_proxy->to_string(), *p_port,
                                     std::placeholders::_3);
        }
        else {
            downloadFunc = ss1x::asio::redirectHttpGet;
        }
    }

    varlisp::detail::http::downloadUrl(p_url->to_string(), max_content, headers, downloadFunc);

    // COLOG_INFO(headers.status_code, headers.http_version);
    Environment ret;
    ret["status_code"] = int64_t(headers.status_code);
    if (!headers.http_version.empty()) {
        ret["http_version"] = string_t(std::move(headers.http_version));
    }

    for (const auto& it : headers) {
        // COLOG_INFO(it.first, ": ", sss::raw_string(it.second));
        // NOTE 最好按字符串保存值——因为header的值可能比较奇葩。
        // 而且，有可能数字以0开头——你保存为int，那么前导的0就丢失了！
        ret[it.first] = string_t(std::move(it.second));
    }
    COLOG_INFO(ret);

    return varlisp::List::makeSQuoteList(string_t(std::move(max_content)),
                                         std::move(ret));
}

}  // namespace varlisp
