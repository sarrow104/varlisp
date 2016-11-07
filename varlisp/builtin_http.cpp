#include "object.hpp"
#include "eval_visitor.hpp"

#include <sss/utlstring.hpp>
#include <sss/iConvpp.hpp>
#include <sss/colorlog.hpp>
#include <sss/encoding.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>

namespace varlisp {

void ensure_utf8(std::string& content, const std::string& encodings)
{
    std::string encoding = sss::Encoding::encodings(content, encodings);
    if (encoding.empty()) {
        encoding = sss::Encoding::dectect(content);
    }

    if (!sss::Encoding::isCompatibleWith(encoding, "utf8")) {
        std::string out;
        sss::iConv ic("utf8", encoding);
        ic.convert(out, content);
        std::swap(out, content);
    }
}


// TODO
// 对于失败的下载，应该告知用户content-length，以及终止在何处(已经接受的bytes数)
// 另外，ensure-utf，应该交给用户，而不是自动完成。
Object eval_http_get(varlisp::Environment& env, const varlisp::List& args)
{
    Object url = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string *p_url = boost::get<std::string>(&url);
    if (!p_url) {
        SSS_POSTION_THROW(std::runtime_error,
                          "http-get requie url for downloading!");
    }

    std::ostringstream oss;
    ss1x::http::Headers headers;

    if (args.length() == 3) {
        const std::string * p_proxy = boost::get<std::string>(&args.tail[0].head);
        if (!p_proxy) {
            SSS_POSTION_THROW(std::runtime_error,
                              "http-get 2nd parameter must be proxy domain string!");
        }
        const int * p_port = boost::get<int>(&args.tail[0].tail[0].head);
        if (!p_port) {
            SSS_POSTION_THROW(std::runtime_error,
                              "http-get 3rd parameter must be proxy port number!");
        }
        ss1x::asio::proxyGetFile(oss, headers, *p_proxy, *p_port, *p_url);
    }
    else {
        ss1x::asio::getFile(oss, headers, *p_url);
    }

    std::string content = oss.str();
    std::string charset = sss::trim_copy(headers.get("Content-Type", "charset"));
    if (!charset.empty()) {
        COLOG_INFO("(http-get: charset from Content-Type = ", sss::raw_string(charset), ")");
    }
    if (charset.empty()) {
        charset = "gb2312,utf8";
    }

    ensure_utf8(content, charset);

    return content;
}


} // namespace varlisp
