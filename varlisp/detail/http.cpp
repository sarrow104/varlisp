#include "http.hpp"

#include <array>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "../builtin_helper.hpp"
#include "../string.h"

namespace varlisp {
namespace detail {
namespace http {

void Environment2ss1x_header(ss1x::http::Headers& header,
                             varlisp::Environment& env,
                             const varlisp::Environment& info)
{
    const char * funcName = __PRETTY_FUNCTION__;
    std::array<Object, 1> objs;
    int id = 0;
    for (auto it = info.begin(); it != info.end(); ++it, ++id) {
        if (it->first == "http_version") {
            header.http_version =
                requireTypedValue<varlisp::string_t>(env, it->second.first, objs[0], funcName, id, DEBUG_INFO)->to_string();
        }
        else {
            header[it->first] =
                requireTypedValue<varlisp::string_t>(env, it->second.first, objs[0], funcName, id, DEBUG_INFO)->to_string();
        }
    }
}

void downloadUrl(
    const std::string& url, std::string& max_content,
    ss1x::http::Headers& headers,
    const std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string& url)>& func,
    int max_test)
{
    boost::system::error_code ec;

    std::ostringstream oss;
    
    do {
        ec = func(oss, headers, url);

        if (headers.status_code != 200) {
            COLOG_ERROR("(: http-status code:", headers.status_code, ")");
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
                oss.str("");
            }
            if (max_content.length() == content_length) {
                break;
            }
        }
    } while (max_test-- > 0);
}
} // namespace http
} // namespace detail
} // namespace varlisp
