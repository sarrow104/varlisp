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

namespace detail {
// void parse
} // namespace detail

REGIST_BUILTIN(
    "http-timeout", 0, 1, eval_http_timeout,
    "; http-timeout 获取/设置网络超时\n"
    "(http-timeout) -> cur-timeout-in-seconds\n"
    "(http-timeout new-timeout-in-seconds) -> old-timeout-in-seconds\n");

/**
 * @brief
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_http_timeout(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "http-timeout";
    if (!args.length()) {
        return int64_t(ss1x::asio::ptc_deadline_timer_wait());
    }
    else {
        int64_t old_timeout = ss1x::asio::ptc_deadline_timer_wait();
        Object tmp;
        const auto* p_timeout = varlisp::requireTypedValue<int64_t>(
            env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);
        ss1x::asio::ptc_deadline_timer_wait() = *p_timeout;
        return old_timeout;
    }
}

REGIST_BUILTIN(
    "http-debug", 0, 1, eval_http_debug,
    "; http-debug 获取/设置当前debug模式\n"
    "(http-debug) -> cur-http-debug-status\n"
    "(http-debug new-http-debug-status) -> old-http-debug-status\n");

/**
 * @brief
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_http_debug(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "http-debug";
    if (args.length() == 0U) {
        return bool(ss1x::asio::ptc_colog_status());
    }
    bool old_status = ss1x::asio::ptc_colog_status();
    Object tmp;
    const auto* p_bool = varlisp::requireTypedValue<bool>(
        env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);
    ss1x::asio::ptc_colog_status() = *p_bool;
    return old_status;
   
}

REGIST_BUILTIN(
    "http-get", 1, 4, eval_http_get,
    "; http-get 获取网络资源\n"
    "(http-get \"url\") -> [<html>, {response}]\n"
    "(http-get \"url\" {request_header}) -> [<html>, {response}]\n"
    "(http-get \"url\" \"proxy-url\" proxy-port-number) -> [<html>, {response}]\n"
    "(http-get \"url\" \"proxy-url\" proxy-port-number {request_header}) -> [<html>, {response}]");

/**
 * @brief
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_http_get(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "http-get";
    std::array<Object, 5> objs;
    const auto* p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_proxy = nullptr;
    const int64_t* p_port = nullptr;
    ss1x::http::Headers request_header;

    if (args.length() >= 3) {
        p_proxy = 
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

        p_port = 
            requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);
    }
    if (args.length() == 2 || args.length() == 4) {
        auto rh_index = args.length() - 1;
        const auto * p_request_header =
            requireTypedValue<varlisp::Environment>(env, args.nth(rh_index), objs[rh_index], funcName, rh_index, DEBUG_INFO);
        varlisp::detail::http::Environment2ss1x_header(request_header, env, *p_request_header);
    }

    ss1x::http::Headers headers;

    std::string max_content;

#if 0
    std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string&, const ss1x::http::Headers&)> downloadFunc;

    if (detail::CookieMgr_t::get_cookie_enable_status()) {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpGetCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     *p_proxy->gen_shared(), *p_port,
                                     std::placeholders::_3,
                                     ss1x::cookie::get,
                                     std::placeholders::_4);
        }
        else {
            downloadFunc = std::bind(ss1x::asio::redirectHttpGetCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     ss1x::cookie::get,
                                     std::placeholders::_4);
        }
    }
    else {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpGet,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     *p_proxy->gen_shared(), *p_port,
                                     std::placeholders::_3,
                                     std::placeholders::_4);
        }
        else {
            downloadFunc = std::bind(ss1x::asio::redirectHttpGet,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     std::placeholders::_4);
        }
    }

    varlisp::detail::http::downloadUrl(*p_url->gen_shared(), max_content, headers, downloadFunc, request_header);
#else
    varlisp::detail::http::downloadUrlCurl(varlisp::detail::http::curl_method_get, *p_url->gen_shared(), max_content, headers, request_header, nullptr);
#endif

    // COLOG_INFO(headers.status_code, headers.http_version);
    Environment ret;
    ret["status_code"] = int64_t(headers.status_code);
    if (!headers.http_version.empty()) {
        ret["http_version"] = string_t(headers.http_version);
    }

    for (const auto& it : headers) {
        // COLOG_INFO(it.first, ": ", sss::raw_string(it.second));
        // NOTE 最好按字符串保存值——因为header的值可能比较奇葩。
        // 而且，有可能数字以0开头——你保存为int，那么前导的0就丢失了！
        ret[it.first] = string_t(it.second);
    }
    COLOG_INFO(ret);

    return varlisp::List::makeSQuoteList(string_t(max_content),
                                         std::move(ret));
}

REGIST_BUILTIN(
    "http-post", 2, 5, eval_http_post,
    "; http-post 上传网络资源\n"
    "(http-post \"url\" content) -> [<html>, {response}]\n"
    "(http-post \"url\" content {request_header}) -> [<html>, {response}]\n"
    "(http-post \"url\" content \"proxy-url\" proxy-port-number) -> [<html>, {response}]\n"
    "(http-post \"url\" content \"proxy-url\" proxy-port-number {request_header}) -> [<html>, {response}]");

/**
 * @brief
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_http_post(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "http-post";
    std::array<Object, 6> objs;
    const auto* p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const auto* p_content =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    const string_t* p_proxy = nullptr;
    const int64_t* p_port = nullptr;
    ss1x::http::Headers request_header;

    if (args.length() >= 4) {
        p_proxy = 
            requireTypedValue<varlisp::string_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);

        p_port = 
            requireTypedValue<int64_t>          (env, args.nth(3), objs[3], funcName, 3, DEBUG_INFO);
    }
    if (args.length() == 3 || args.length() == 5) {
        auto rh_index = args.length() - 1;
        const auto * p_request_header =
            requireTypedValue<varlisp::Environment>(env, args.nth(rh_index), objs[rh_index], funcName, rh_index, DEBUG_INFO);
        varlisp::detail::http::Environment2ss1x_header(request_header, env, *p_request_header);
    }

    ss1x::http::Headers headers;

    std::string max_content;

#if 0
    std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string&, const ss1x::http::Headers&)> downloadFunc;

    if (detail::CookieMgr_t::get_cookie_enable_status()) {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpPostCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     *p_proxy->gen_shared(), *p_port,
                                     std::placeholders::_3,
                                     *p_content->gen_shared(),
                                     ss1x::cookie::get,
                                     std::placeholders::_4);
        }
        else {
            downloadFunc = std::bind(ss1x::asio::redirectHttpPostCookie,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     *p_content->gen_shared(),
                                     ss1x::cookie::get,
                                     std::placeholders::_4);
        }
    }
    else {
        if (p_proxy) {
            downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpPost,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     *p_proxy->gen_shared(), *p_port,
                                     std::placeholders::_3,
                                     *p_content->gen_shared(),
                                     std::placeholders::_4);
        }
        else {
            downloadFunc = std::bind(ss1x::asio::redirectHttpPost,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     *p_content->gen_shared(),
                                     std::placeholders::_4);
        }
    }

    varlisp::detail::http::downloadUrl(*p_url->gen_shared(), max_content, headers, downloadFunc, request_header);
#else
    varlisp::detail::http::downloadUrlCurl(varlisp::detail::http::curl_method_post, *p_url->gen_shared(), max_content, headers, request_header, &*p_content->gen_shared());
#endif

    // COLOG_INFO(headers.status_code, headers.http_version);
    Environment ret;
    ret["status_code"] = int64_t(headers.status_code);
    if (!headers.http_version.empty()) {
        ret["http_version"] = string_t(headers.http_version);
    }

    for (const auto& it : headers) {
        // COLOG_INFO(it.first, ": ", sss::raw_string(it.second));
        // NOTE 最好按字符串保存值——因为header的值可能比较奇葩。
        // 而且，有可能数字以0开头——你保存为int，那么前导的0就丢失了！
        ret[it.first] = string_t(it.second);
    }
    COLOG_INFO(ret);

    return varlisp::List::makeSQuoteList(string_t(max_content),
                                         std::move(ret));
}

}  // namespace varlisp
