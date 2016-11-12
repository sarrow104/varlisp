#include "object.hpp"
#include "builtin_helper.hpp"

#include <sss/colorlog.hpp>
#include <sss/encoding.hpp>
#include <sss/iConvpp.hpp>
#include <sss/utlstring.hpp>
#include <sss/debug/value_msg.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>

namespace varlisp {

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
    const char * funcName = "http-get";
    Object url;
    const std::string* p_url = getTypedValue<std::string>(env, args.head, url);
    if (!p_url) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(", funcName, ": requie downloading url as 1st argument !)");
    }

    Object proxy;             
    const std::string* p_proxy = 0;
    Object port;
    const int* p_port = 0;

    if (args.length() == 3) {
        p_proxy = getTypedValue<std::string>(env, args.tail[0].head, proxy);
        if (!p_proxy) {
            SSS_POSTION_THROW(
                std::runtime_error,
                "(", funcName, ": 2nd parameter must be proxy domain string!)");
        }
        p_port = getTypedValue<int>(env, args.tail[0].tail[0].head, port);
        if (!p_port) {
            SSS_POSTION_THROW(
                std::runtime_error,
                "(", funcName, ": 3rd parameter must be proxy port number!)");
        }
    }

    ss1x::http::Headers headers;

    std::string max_content;

    int max_test = 5;
    do {
        std::ostringstream oss;
        if (p_proxy) {
            ss1x::asio::proxyGetFile(oss, headers, *p_proxy, *p_port, *p_url);
        }
        else {
            ss1x::asio::getFile(oss, headers, *p_url);
        }
        if (headers.status_code != 200) {
            COLOG_ERROR("(",funcName, ": http-status code:", headers.status_code, ")");
            continue;
        }
        if (unsigned(oss.tellp()) > max_content.length()) {
            max_content = oss.str();
        }
        std::string content_length_str = headers.get("Content-Length");
        sss::trim(content_length_str);
        COLOG_DEBUG(SSS_VALUE_MSG(content_length_str));
        COLOG_DEBUG(SSS_VALUE_MSG(max_content.length()));
        if (content_length_str.empty()) {
            break;
        }
        if (max_content.length() == sss::string_cast<unsigned int>(content_length_str)) {
            break;
        }
    } while (max_test-- > 0);

    // http-headers? usage?
    // std::string charset =
    //     sss::trim_copy(headers.get("Content-Type", "charset"));
    // if (!charset.empty()) {
    //     COLOG_INFO("(http-get: charset from Content-Type = ",
    //                sss::raw_string(charset), ")");
    // }

    return max_content;
}

}  // namespace varlisp
