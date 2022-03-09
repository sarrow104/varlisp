#include "http.hpp"
#include "gfw_base.hpp"
#include "consumer.hpp"
#include "config.hpp"

#include <curl/curl.h>

#include <array>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/macro/defer.hpp>

#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>
#include <string_view>
#include <tuple>

#include "../interpreter.hpp"
#include "../builtin_helper.hpp"
#include "url.hpp"

namespace varlisp::detail {

int64_t get_value_with_default(std::string name, int64_t def);

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

std::tuple<std::string, std::string> urlSplitByPath(std::string url) {
    std::string_view sv{url};
    if (trim_positive_if(sv, [](char c)->bool { return std::isalpha(c); }) &&
        trim_one(sv, ':') &&
        trim_times(sv, 2, '/') &&
        trim_positive_if(sv, [](char c)->bool { return c != '/'; }))
    {
        auto offset = std::distance(url.data(), const_cast<char*>(sv.data()));
        return std::make_tuple(url.substr(0, offset), url.substr(offset));
    }

    return std::make_tuple(url, "");
}

struct RestClientInitWrapper {
    RestClientInitWrapper() {
        COLOG_INFO("RestClient::init()");
        RestClient::init();
        RestClient::get("123");
    }
    ~RestClientInitWrapper() {
        COLOG_INFO("RestClient::disable()");
        RestClient::disable();
    }
};

const char * get_method_name(curl_method_t method) {
    switch (method) {
        case curl_method_get:
            return "method_get";

        case curl_method_head:
            return "method_head";

        case curl_method_del: // delete
            return "method_delete";

        case curl_method_options:
            return "method_options";

        case curl_method_patch:
            return "method_patch";

        case curl_method_post:
            return "method_post";

        default:
            return "<invalid>";
    }
}

void downloadUrlCurlImpl(
    curl_method_t method,
    const std::string& url,
	std::string& max_content,
    ss1x::http::Headers& respond_header,
    const ss1x::http::Headers& request_header,
    std::string* ptr_post_body)
{
    static auto proxyMgr =
        varlisp::detail::gfw_base::make_mgr(
            config::gfw_rule_mgr_methd,
            config::get_omegaGfwRulePath());

    static auto proxyInfo = proxyMgr->get_host() + ":" + std::to_string(proxyMgr->get_port());
    static RestClientInitWrapper wrapRestClient;

    bool need_proxy = proxyMgr->need_proxy(url);

	// get a connection object
    auto [urlPrefix, urlPath] = urlSplitByPath(url);
    COLOG_INFO(SSS_VALUE_MSG(urlPrefix), SSS_VALUE_MSG(urlPath));
    auto conn = std::make_shared<RestClient::Connection>(urlPrefix);
	//RestClient::Connection* conn = new RestClient::Connection(urlPrefix);
    //SSS_DEFER( delete conn );

	// configure basic auth
	// conn->SetBasicAuth("WarMachine68", "WARMACHINEROX");

	// set connection timeout to 5s
    auto timeout_sec = varlisp::detail::get_value_with_default("curl-timeout", 60);
	conn->SetTimeout(timeout_sec);

	// set custom user agent
	// (this will result in the UA "foo/cool restclient-cpp/VERSION")
	conn->SetUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:90.0) Gecko/20100101 Firefox/90.0");

	// and limit the number of redirects (default is -1, unlimited)
	conn->FollowRedirects(true, 3);

    // NOTE 2021-12-10
    // 1. 以下启用 cookie的代码，没有实际 通过 CloadFlare 校验的作用
    // 2. 有"概率"导致 segmentfault ——原因不明——出错的时候，curlHandle 指针非nullptr
    if (false) {
        auto curlHandle = reinterpret_cast<CURL*>(conn.get());

        if (curlHandle) {
            const char * cookiesPath = "/tmp/varlisp-cookies.txt";

            COLOG_INFO(curlHandle);
            curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, cookiesPath); // for reading
            COLOG_INFO();
            curl_easy_setopt(curlHandle, CURLOPT_COOKIEJAR, cookiesPath); // for writing
            COLOG_INFO();
        } else {
            COLOG_ERROR("nullptr");
        }

    }

	// set headers
	RestClient::HeaderFields headers;
	for (const auto& p: request_header) {
		headers[p.first] = p.second;
	}
    if (!headers.contains("Accept")) {
        headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
    }

	conn->SetHeaders(headers);

	//	// append additional headers
	//	conn->AppendHeader("X-MY-HEADER", "foo")

	//	// if using a non-standard Certificate Authority (CA) trust file
	//	conn->SetCAInfoFilePath("/etc/custom-ca.crt")

	//	RestClient::Response r = conn->get("/get");
	//	RestClient::Response r = conn->head("/get");
	//	RestClient::Response r = conn->del("/delete");
	//	RestClient::Response r = conn->options("/options");

	//	// set different content header for POST, PUT and PATCH
	//	conn->AppendHeader("Content-Type", "application/json");
	//	RestClient::Response r = conn->post("/post", "{\"foo\": \"bla\"}");
	//	RestClient::Response r = conn->put("/put", "application/json", "{\"foo\": \"bla\"}");
	//	RestClient::Response r = conn->patch("/patch", "text/plain", "foobar");

	//	// deinit RestClient. After calling this you have to call RestClient::init()
	//	// again before you can use it
	//	RestClient::disable();

    if (need_proxy) {
        COLOG_INFO(SSS_VALUE_MSG(proxyInfo));
        // set CURLOPT_PROXY
        // conn->SetProxy("127.0.0.1:7890");
        /* or you can set it without the protocol scheme and http:// will be prefixed by default */
        conn->SetProxy(proxyInfo);
    }

    /* the following request will be tunneled through the proxy */
    RestClient::Response res;

    switch (method) {
        case curl_method_get:
            res = conn->get(urlPath);
            break;

        case curl_method_head:
            res = conn->head(urlPath);
            break;

        case curl_method_del: // delete
            res = conn->del(urlPath);
            break;

        case curl_method_options:
            res = conn->options(urlPath);
            break;

        case curl_method_patch:
            if (ptr_post_body == nullptr) {
                COLOG_ERROR("null ptr_post_body");
                respond_header.status_code = 0;
                return;
            }
            res = conn->patch(urlPath, *ptr_post_body);
            break;

        case curl_method_post:
            if (ptr_post_body == nullptr) {
                COLOG_ERROR("null ptr_post_body");
                respond_header.status_code = 0;
                return;
            }
            res = conn->post(urlPath, *ptr_post_body);
            break;

        default:
            COLOG_ERROR("do not support method", method);
            respond_header.status_code = 0;
            return;
    }

    /* the following request will be tunneled through the proxy */

    for (const auto& p: res.headers) {
        respond_header.insert(std::make_pair(p.first, p.second));
    }
    respond_header.status_code = res.code;
    if (res.code / 100 != 2) {

        COLOG_INFO("code:", res.code,
                   get_method_name(method), url,
                   SSS_VALUE_MSG(request_header), SSS_VALUE_MSG(respond_header),
                   "size:", res.body.size());
    }

    max_content = res.body;
}

void downloadUrlCurl(
    curl_method_t method,
    const std::string& url,
	std::string& max_content,
    ss1x::http::Headers& respond_header,
    const ss1x::http::Headers& request_header,
    std::string* ptr_post_body,
    int max_test)
{
    auto newUrl = url;
    if (request_header.has("Referer")) {
        COLOG_INFO(newUrl, request_header.get("Referer"));
        varlisp::detail::url::full_of(newUrl, request_header.get("Referer"));
    }


    auto request_header_tmp = request_header;
    while (max_test-- > 0) {
        respond_header.status_code = 0;
        downloadUrlCurlImpl(method, newUrl, max_content, respond_header, request_header_tmp, ptr_post_body);
        if (respond_header.status_code / 100 == 2) {
            break;
        }
        if (respond_header.status_code / 100 == 4) {
            // HTTP/1.1 403 Forbidden: present
            // Pragma: no-cache
            // Server: Tengine
            // Strict-Transport-Security: max-age=5184000
            // Timing-Allow-Origin: *
            // Transfer-Encoding: chunked
            // Via: vcache9.cn3425[,403003]
            // X-Tengine-Error: denied by Referer ACL

            if (request_header_tmp.has("Referer")) {
                request_header_tmp.erase("Referer");
            } else {
                break;
            }
        }
    }
}

void downloadUrl(
    const std::string& url, std::string& max_content,
    ss1x::http::Headers& headers,
    const std::function<boost::system::error_code(
        std::ostream&, ss1x::http::Headers&, const std::string& url, const ss1x::http::Headers&)>& func,
    const ss1x::http::Headers& request_header,
    int max_test)
{
    boost::system::error_code ec;
    std::string cur_url = url;

    std::ostringstream oss;

    ss1x::http::Headers request_header2;
    auto* p_req = const_cast<ss1x::http::Headers*>(&request_header);

    do {
        oss.str("");
        COLOG_DEBUG(SSS_VALUE_MSG(cur_url), *p_req);
        ec = func(oss, headers, cur_url, *p_req);

        if (headers.status_code == 404) {
            break;
        }
        if ((headers.status_code / 100) != 2) {
            COLOG_ERROR("(", sss::raw_string(cur_url), ": http-status code:", headers.status_code, ")");
            for (const auto& item : headers) {
                std::cerr << "header : " << item.first << ": " << item.second
                    << std::endl;
            }

            if ((headers.status_code == 301 || headers.status_code == 302) && headers.has("Location"))
            {
                cur_url = headers["Location"];
                continue;
            }
            if (headers.status_code == 403 && p_req->has("Referer")) {
                request_header2 = request_header;
                request_header2.unset("Referer");
                p_req = &request_header2;
                continue;
            }
            if (headers.status_code == 0) {
                // NOTE zero means nothing happend or totally failed;
                // so try again
                continue;
            }
            break;
        }
        if (!headers.has("Content-Length")) {
            if (ec == boost::asio::error::eof) {
                max_content = oss.str();
                break;
            }
            COLOG_ERROR(ec, "; loop = ", max_test);
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
            if (actual_recieved > content_length && !headers.has("Content-Encoding")) { //  "gzip"
                COLOG_DEBUG(SSS_VALUE_MSG(actual_recieved), '>',
                            SSS_VALUE_MSG(content_length));
                break;
            }
            if (actual_recieved < content_length) {
                COLOG_DEBUG(SSS_VALUE_MSG(actual_recieved), '<',
                            SSS_VALUE_MSG(content_length));
                // retry
            }
            if (actual_recieved > max_content.length() &&
                (actual_recieved <= content_length || headers.has("Content-Encoding"))) {
                max_content = oss.str();
            }
            if (max_content.length() == content_length || (actual_recieved > content_length && headers.has("Content-Encoding"))) {
                break;
            }
        }
    } while (max_test-- > 0);
}

} // namespace http
} // namespace varlisp::detail
