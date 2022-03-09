#include <functional>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <sss/colorlog.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>

#include "../environment.hpp"

namespace varlisp::detail::http {

void Environment2ss1x_header(ss1x::http::Headers& header,
                             varlisp::Environment& env,
                             const varlisp::Environment& info);

enum curl_method_t : uint8_t {
    curl_method_get,
    curl_method_post,
    curl_method_head,
    curl_method_del, // delete
    curl_method_options,
    curl_method_patch,
};

void downloadUrlCurl(
    curl_method_t method,
    const std::string& url,
	std::string& max_content,
    ss1x::http::Headers& respond_header,
    const ss1x::http::Headers& request_header,
    std::string* ptr_post_body,
    int max_test = 5);

void downloadUrl(
    const std::string& url, std::string& max_content,
    ss1x::http::Headers& headers,
    const std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string& url, const ss1x::http::Headers&)>& func,
    const ss1x::http::Headers& request_header,
    int max_test = 5);

} // namespace varlisp::detail::http
