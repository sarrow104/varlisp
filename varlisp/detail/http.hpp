#include <functional>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <sss/colorlog.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>

namespace varlisp {

namespace detail {
namespace http {

void downloadUrl(
    const std::string& url, std::string& max_content,
    ss1x::http::Headers& headers,
    const std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string& url)>& f,
    int max_test = 5);

} // namespace http

} // namespace detail
} // namespace varlisp