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

#include "builtin_helper.hpp"
#include "object.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN(
    "http-get", 1, 3, eval_http_get,
    "(http-get \"url\") -> \"<html>\"\n"
    "(http-get \"url\" \"proxy-url\" proxy-port-number) -> \"<html>\"");

// TODO
// 对于失败的下载，应该告知用户content-length，以及终止在何处(已经接受的bytes数)
// 另外，ensure-utf，应该交给用户，而不是自动完成。
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
    const int* p_port = 0;

    if (args.length() == 3) {
        p_proxy = getTypedValue<string_t>(env, detail::cadr(args), proxy);
        if (!p_proxy) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 2nd parameter must be proxy domain string!)");
        }
        p_port = getTypedValue<int>(env, detail::caddr(args), port);
        if (!p_port) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 3rd parameter must be proxy port number!)");
        }
    }

    ss1x::http::Headers headers;

    string_t max_content;

    boost::system::error_code ec;

    int max_test = 5;
    do {
        std::ostringstream oss;
        if (p_proxy) {
            ec = ss1x::asio::proxyRedirectHttpGet(oss, headers,
                                                  p_proxy->to_string(), *p_port,
                                                  p_url->to_string());
            // ss1x::asio::proxyGetFile(oss, headers, p_proxy->to_string(),
            // *p_port, p_url->to_string());
        }
        else {
            // ss1x::asio::getFile(oss, headers, p_url->to_string());
            ec = ss1x::asio::redirectHttpGet(oss, headers, p_url->to_string());
        }

        if (headers.status_code != 200) {
            COLOG_ERROR("(", funcName, ": http-status code:",
                        headers.status_code, ")");
            for (const auto& item : headers) {
                std::cerr << "header : " << item.first << ": " << item.second
                          << std::endl;
            }
            continue;
        }
        if (!headers.has("Content-Length")) {
            if (ec == boost::asio::error::eof) {
                max_content = oss.str();
                break;
            }
        }
        else {
            std::string content_length_str = headers.get("Content-Length");
            sss::trim(content_length_str);
            if (content_length_str.empty()) {
                break;
            }
            size_t content_length =
                sss::string_cast<unsigned int>(content_length_str);
            COLOG_DEBUG(SSS_VALUE_MSG(oss.str().length()));
            size_t actual_recieved = oss.tellp();
            if (actual_recieved > content_length) {
                COLOG_DEBUG(SSS_VALUE_MSG(actual_recieved), '>',
                            SSS_VALUE_MSG(content_length));
                break;
            }
            else if (actual_recieved < content_length) {
                COLOG_DEBUG(SSS_VALUE_MSG(actual_recieved), '<',
                            SSS_VALUE_MSG(content_length));
                // retry
            }
            if (actual_recieved > max_content.length() &&
                actual_recieved <= content_length) {
                max_content = oss.str();
            }
            if (max_content.length() == content_length) {
                break;
            }
        }
    } while (max_test-- > 0);

    // COLOG_INFO(headers.status_code, headers.http_version);
    Environment ret;
    ret["status_code"] = int(headers.status_code);
    if (!headers.http_version.empty()) {
        ret["http_version"] = string_t(std::move(headers.http_version));
    }

    for (const auto& it : headers) {
        // COLOG_INFO(it.first, ": ", sss::raw_string(it.second));
        if (sss::is_all(it.second, static_cast<int (*)(int)>(std::isdigit))) {
            ret[it.first] = sss::string_cast<int>(it.second);
        }
        else {
            ret[it.first] = string_t(std::move(it.second));
        }
    }
    COLOG_INFO(ret);

    return varlisp::List::makeSQuoteList(std::move(max_content),
                                         std::move(ret));
}

}  // namespace varlisp
