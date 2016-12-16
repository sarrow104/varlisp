#include <cctype>

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
    Object url;
    const string_t* p_url =
        getTypedValue<string_t>(env, detail::car(args), url);
    if (!p_url) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requie downloading url as 1st argument !)");
    }

    Object proxy;
    const string_t* p_proxy = 0;
    Object port;
    const int64_t* p_port = 0;

    if (args.length() == 3) {
        p_proxy = getTypedValue<string_t>(env, detail::cadr(args), proxy);
        if (!p_proxy) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 2nd parameter must be proxy domain string!)");
        }
        p_port = getTypedValue<int64_t>(env, detail::caddr(args), port);
        if (!p_port) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 3rd parameter must be proxy port number!)");
        }
    }

    ss1x::http::Headers headers;

    std::string max_content;

    if (p_proxy) {
        varlisp::detail::http::downloadUrl(p_url->to_string(), max_content, headers,
                                           std::bind(ss1x::asio::proxyRedirectHttpGet,
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     p_proxy->to_string(), *p_port,
                                                     std::placeholders::_3));
    }
    else {
        varlisp::detail::http::downloadUrl(p_url->to_string(), max_content, headers,
                                           ss1x::asio::redirectHttpGet);
    }

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
#if 0
        if (sss::is_all(it.second, static_cast<int (*)(int)>(std::isdigit))) {
            try {
                ret[it.first] = sss::string_cast<int64_t>(it.second);
            }
            catch (std::runtime_error& e) {
                ret[it.first] = string_t(std::move(it.second));
            }
        }
        else {
            ret[it.first] = string_t(std::move(it.second));
        }
#else
        ret[it.first] = string_t(std::move(it.second));
#endif
    }
    COLOG_INFO(ret);

    return varlisp::List::makeSQuoteList(string_t(std::move(max_content)),
                                         std::move(ret));
}

}  // namespace varlisp
