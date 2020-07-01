#include "html.hpp"

#include <re2/re2.h>

#include <gq/QueryUtil.h>
#include <gq/DocType.h>

#include <sss/string_view.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/utlstring.hpp>
#include <sss/algorithm.hpp>

#include <ss1x/asio/headers.hpp>
#include <ss1x/asio/utility.hpp>
#include <ss1x/uuid/sha1.hpp>

#include "http.hpp"
#include "url.hpp"

namespace varlisp {
namespace detail {
namespace html {

struct htmlEntityEscape_t : public sss::string_view
{
    explicit htmlEntityEscape_t(sss::string_view s)
        : sss::string_view(s)
    {}
	void print(std::ostream& o) const
    {
		// 0xA0 -> &#160 -> &nbsp;
		// 对应utf8，是两个字节，分别是
		// 0xC2, 0xA0
        for (const char * p_ch = this->begin(); p_ch < this->end(); ++p_ch)
        {
            switch (*p_ch) {
                case '&':
                    o << "&amp;";
                    break;

                case '<':
                    o << "&lt;";
                    break;

                case '>':
                    o << "&gt;";
                    break;

				case '"':
					o << "&quot;";
					break;

                default:
					if (p_ch + 1 < this->end() && std::memcmp(p_ch, "\xC2\xA0", 2) == 0) {
						o << "&nbsp;";
						++p_ch;
					}
					else {
						o << *p_ch;
					}
                    break;
            }
        }
    }
};

inline std::string getAttribute(GumboNode* apNode, const std::string& key)
{
    for (unsigned int i = 0; i < CQueryUtil::attrNum(apNode); i++) {
        GumboAttribute* attr = CQueryUtil::nthAttr(apNode, i);
        if (key == attr->name) {
            return attr->value;
        }
    }
    return "";
}

inline htmlEntityEscape_t htmlEntityEscape(sss::string_view s)
{
    return htmlEntityEscape_t{s};
}

inline std::ostream& operator << (std::ostream&o, const htmlEntityEscape_t& h)
{
    h.print(o);
    return o;
}

bool is_url_same_origin(sss::string_view left, sss::string_view right)
{
    sss::string_view::const_reverse_iterator rit_l = left.rbegin();
    sss::string_view::const_reverse_iterator rit_r = right.rbegin();

    sss::string_view::const_reverse_iterator rend_l = left.rend();
    sss::string_view::const_reverse_iterator rend_r = right.rend();

    size_t matched_dot_cnt = 0;
    for (; rit_l != rend_l && rit_r != rend_r; ++rit_l, ++rit_r)
    {
        if (*rit_l != *rit_r)
        {
            break;
        }
        if (*rit_l == '.')
        {
            ++matched_dot_cnt;
        }
    }

    return matched_dot_cnt >= 2;
}

static void trim_to(std::string& str, char mark)
{
    auto pos = str.find(mark);
    if (pos != std::string::npos) {
        str.resize(pos);
    }
}

std::string getResourceAuto(const std::string& output_dir, const std::string& url,
                            resource_manager_t& rs_mgr, const ss1x::http::Headers& request_header,
                            const std::string& proxy_domain, int proxy_port)
{
    std::string max_content;
    ss1x::http::Headers headers;
    std::string newUrl = url;
    if (request_header.has("Referer")) {
        varlisp::detail::url::full_of(newUrl, request_header.get("Referer"));
    }

    std::function<boost::system::error_code(
        std::ostream&,
        ss1x::http::Headers&,
        const std::string& url,
        const ss1x::http::Headers&)> downloadFunc;

    if (!proxy_domain.empty() && proxy_port > 0) {
        downloadFunc = std::bind(ss1x::asio::proxyRedirectHttpGet,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 proxy_domain, proxy_port,
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

    detail::http::downloadUrl(newUrl, max_content, headers,
                              downloadFunc, request_header);

    if (headers.status_code / 100 != 2) { // 200
        COLOG_ERROR(url);
        rs_mgr[url] = local_info_t{"", 0, fs_ERROR};
        return "";
    }
    // NOTE 有类图片资源，是形如:
    // http://m.tiebaimg.com/timg?wapp&q....
    // 即，真正的图片url信息，是保存在?后面，作为url参数存在的。
    // 此种情况，可以说，基本无法分析；
    // 那如何能得到唯一的地址名呢？
    // hash。
    // 可以针对内容或者url本身进行hash。
    //
    // 完全使用hash命名，可以解决无意义图片名的问题。但是，万一图片名字是有意义的呢？
    // 甚至，就是表示简单的图片顺序呢？这样的话，信息也丢失了。
    // 关键在于，如果原始图片的名字，就是十六进制字符表示的hash串，那么，你如何与正
    // 常的图片名，区分开呢？上分词？
    std::string raw_url;
    auto question_mark_pos = url.find('?');
    if (question_mark_pos != std::string::npos) {
        raw_url = url.substr(0, question_mark_pos);
    }
    else {
        raw_url = url;
    }

    std::string fnameSuffix = sss::path::suffix(raw_url);
    if (headers.has("Content-Type")) {
        // NOTE eg. Content-Type: "image/svg+xml"
        fnameSuffix = "." + sss::path::basename(headers["Content-Type"]);
        trim_to(fnameSuffix, ';');
        trim_to(fnameSuffix, '+');
    }

    std::string output_path = output_dir;

#if 0
    std::string fname = sss::path::no_suffix(sss::path::basename(raw_url));
    sss::path::append(output_path, fname + fnameSuffix);
#else
    sss::path::append(output_path,
                      sss::to_hex(ss1x::uuid::sha1::fromBytes(
                          max_content.c_str(), max_content.size())) +
                          fnameSuffix);
#endif

    if (sss::path::file_exists(output_path) == sss::PATH_TO_FILE) {
        rs_mgr[url] = {output_path, max_content.size(), fs_EXIST};
        return output_path;
    }
    std::ofstream ofs(output_path);
    if (!ofs.good()) {
        rs_mgr[url] = {output_path, max_content.size(), fs_ERROR};
        COLOG_ERROR("open file ", output_path, " to write, error.");
        return output_path;
    }
    ofs << max_content;

    COLOG_INFO(url, " -> ", output_path, "; ", max_content.size(), " bytes.");
    rs_mgr[url] = {output_path, max_content.size(), fs_DONE};
    return output_path;
}

template<typename Container, typename ValueT>
bool contains(const Container& c, const ValueT& tar)
{
    for (const auto& i : c) {
        if (tar == i) {
            return true;
        }
    }
    return false;
}

void gumbo_rewrite_outterHtml(std::ostream& o, GumboNode* apNode,
                              CQueryUtil::CIndenter& ind,
                              const std::string& output_dir,
                              resource_manager_t& rs_mgr,
                              const ss1x::http::Headers& request_header,
                              const std::string& proxy_domain,
                              int proxy_port,
                              bool pre_mode = false)
{
    switch (apNode->type)
    {
        case GUMBO_NODE_TEXT:
            if (!pre_mode && CQueryUtil::childNum(apNode->parent) >= 2) {
                o << ind;
            }
            if (CQueryUtil::eleTagID(apNode->parent) == GUMBO_TAG_SCRIPT) {
                o << CQueryUtil::getText(apNode);
            }
            else {
                o << htmlEntityEscape(CQueryUtil::getText(apNode));
            }
            break;

        case GUMBO_NODE_ELEMENT:
            {
                static std::vector<std::string> neat_print_tag_list {"pre", "span", "a"};
                const std::string tagName = CQueryUtil::tagName(apNode);

                bool is_self_close = CQueryUtil::isSelfCloseTag(apNode);
                bool is_neat_print = pre_mode || contains(neat_print_tag_list, tagName) ||
                                     (CQueryUtil::childNum(apNode) == 1 &&
                                      CQueryUtil::childNum(CQueryUtil::nthChild(
                                          apNode, 0)) == 0);

                if (tagName == "pre") {
                    pre_mode = true;
                }

                if (!pre_mode) {
                    o << ind;
                }
                o << "<" << tagName;
                for (size_t i = 0; i < CQueryUtil::attrNum(apNode); ++i) {
                    std::string attrName = CQueryUtil::nthAttr(apNode, i)->name;
                    if (attrName == "href" || attrName == "src") {
                        // a.href 外链
                        // img.src 内部资源
                        // NOTE 链接的重写，需要如下信息：
                        // 1. 目标链接；
                        // 2. 目标链接如果是相对，则需要a.base; b. 原始url分析；
                        // 3. 如何处理重复的URL？
                        // 因为，可能需要分开处理gpnode的不同部分，所有需要一个
                        // 统一的管理器。
                        //
                        // 另外，下载器，也分为通过proxy，和不通过proxy；
                        // 因此，也最好做一个透明处理。
                        //
                        // 先假设最简单的情况；
                        // o << " " << attrName << "=\""
                        //     << htmlEntityEscape(CQueryUtil::nthAttr(apNode, i)->value) << "\"";
                        // NOTE
                        // <link rel="stylesheet" href="http://static.blog.csdn.net/css/blog_code.css"/>
                        // <script src="http://static.blog.csdn.net/scripts/jquery.js" type="text/javascript"></script>
                        // css的问题在于，部分服务器，使用了css"融合"技术，即同一个链接，虽然获取到的是一个css文件，
                        // 但实际上是由后端，将多个css组合后，再发给浏览器的。此时，css的文件名，很难看……
                        // 另外一种风格，就是 http://static.blog.csdn.net/skin/default/css/style.css?v=1.1
                        // 这种，附带版本号的东西了。
                        //
                    }
                    if ((tagName == "img" && attrName == "src") ||
                        (tagName == "link" && attrName == "href" && detail::html::getAttribute(apNode, "rel") == "stylesheet"))
                    {
                        std::string url = CQueryUtil::nthAttr(apNode, i)->value;

                        bool is_inline_href = sss::is_begin_with(url, "data:");

                        // NOTE https://www.jb51.net/css/41981.html
                        // date:,data:image/png,data:image/jpeg!
                        if (!is_inline_href &&
                            !url.empty() && rs_mgr.find(url) == rs_mgr.end()) {
                            //std::cout << url.substr(0,6) << std::endl;
                            std::cout << url << std::endl;
                            getResourceAuto(output_dir, url, rs_mgr, request_header, proxy_domain, proxy_port);
                        }

                        if (!is_inline_href && !url.empty() && rs_mgr[url].fsize && rs_mgr[url].is_ok()) {
                            o << " " << attrName << "=\""
                                << htmlEntityEscape(sss::path::basename(rs_mgr[url].path)) << "\"";
                        }
                        else {
                            o << " " << attrName << "=\""
                                << htmlEntityEscape(url) << "\"";
                        }
                    }
                    else if (attrName == "style")
                    {
                        COLOG_DEBUG(attrName);
                        std::string style_str = CQueryUtil::nthAttr(apNode, i)->value;
                        // TODO
                        // <span class="LinkCard-backdrop" style="background-image:url(https://pic4.zhimg.com/v2-7d66f99141b9c5ccad48fb4ee54d8bbf_ipico.jpg)"></span>
                        static RE2 re(R"(url\((.+?)\))");
                        RE2::FullMatch(style_str, re);
                        std::ostringstream oss;

                        std::vector<re2::StringPiece> sub_matches;
                        // TODO 解析reg字符串，看捕获数量。
                        sub_matches.resize(2);
                        uint64_t offset = 0;
                        while (offset < style_str.size() &&
                               re.Match(style_str, offset, style_str.size(), RE2::UNANCHORED,
                                                  &sub_matches[0], sub_matches.size()))
                        {
                            oss << sss::string_view(style_str.data() + offset, sub_matches[0].data() - (style_str.data() + offset));

                            oss << "url(";

                            std::string url{sub_matches[1].data(), sub_matches[1].size()};
                            COLOG_DEBUG(SSS_VALUE_MSG(url));

                            // NOTE https://www.jb51.net/css/41981.html
                            // date:,data:image/png,data:image/jpeg!
                            if (!url.empty() && rs_mgr.find(url) == rs_mgr.end()) {
                                //std::cout << url.substr(0,6) << std::endl;
                                std::cout << url << std::endl;
                                getResourceAuto(output_dir, url, rs_mgr, request_header, proxy_domain, proxy_port);
                            }

                            COLOG_DEBUG(rs_mgr[url].fsize, rs_mgr[url].is_ok(), sss::path::basename(rs_mgr[url].path));

                            if (!url.empty() && rs_mgr[url].fsize && rs_mgr[url].is_ok()) {
                                oss << htmlEntityEscape(sss::path::basename(rs_mgr[url].path));
                            }
                            else {
                                oss << htmlEntityEscape(url);
                            }

                            oss << ")";

                            offset = sub_matches[0].data() - style_str.data() + sub_matches[0].size();
                        }
                        oss << sss::string_view(style_str.data() + offset, style_str.size() - offset);

                        o << " " << attrName << "=\"" << oss.str() << "\"";
                    }
                    else {
                        o << " " << attrName << "=\""
                            << htmlEntityEscape(CQueryUtil::nthAttr(apNode, i)->value) << "\"";
                    }
                }
                if (CQueryUtil::childNum(apNode))
                {
                    o << ">";
                    if (!is_neat_print) {
                        o << std::endl;
                    }
                    CQueryUtil::CIndentHelper ih(ind);
                    for (size_t i = 0; i < CQueryUtil::childNum(apNode); ++i)
                    {
                        if (is_neat_print) {
                            CQueryUtil::CIndenter indInner(ind.get().c_str());
                            gumbo_rewrite_outterHtml(
                                o, CQueryUtil::nthChild(apNode, i), indInner,
                                output_dir, rs_mgr, request_header, proxy_domain, proxy_port, pre_mode);
                        }
                        else {
                            gumbo_rewrite_outterHtml(
                                o, CQueryUtil::nthChild(apNode, i), ind,
                                output_dir, rs_mgr, request_header, proxy_domain, proxy_port, pre_mode);
                            o << "\n";
                        }
                    }
                }

                if (is_self_close) {
                    o << "/>";
                }
                else {
                    if (!CQueryUtil::childNum(apNode)) {
                        o << ">";
                    }
                    else {
                        if (!is_neat_print) {
                            o << ind;
                        }
                    }
                    o << "</" << tagName << ">";
                }
            }
            break;

        case GUMBO_NODE_CDATA:
            if (!pre_mode) {
                o << ind;
            }
            o << "<!CDATA[" << CQueryUtil::getText(apNode) << "]]>";
            break;

        case GUMBO_NODE_COMMENT:
            if (!pre_mode) {
                o << ind;
            }
            o << "<!--" << CQueryUtil::getText(apNode) << "-->";
            break;

        case GUMBO_NODE_TEMPLATE:
            if (!pre_mode) {
                o << ind;
            }
            o << "<!-- GUMBO_NODE_TEMPLATE not implement -->";
            break;

        case GUMBO_NODE_DOCUMENT:
            if (!pre_mode) {
                o << ind;
            }
            o << CDocType(&apNode->v.document) << std::endl;
            for (size_t i = 0; i < CQueryUtil::childNum(apNode); i++)
            {
                gumbo_rewrite_outterHtml(
                    o, CQueryUtil::nthChild(apNode, i), ind, output_dir, rs_mgr,
                    request_header, proxy_domain, proxy_port, pre_mode);
            }
            break;

        case GUMBO_NODE_WHITESPACE:
            if (pre_mode) {
                o << CQueryUtil::getText(apNode);
            }
            break;

        default:
            {
                std::ostringstream oss;
                oss << "unknown GumboNodeType value " << apNode->type;
                throw oss.str();
            }
            break;
    }
}

void gumbo_rewrite_impl(int fd, const gumboNode& g,
                        const std::string& output_dir, resource_manager_t& rs_mgr,
                        const ss1x::http::Headers& request_header,
                        const std::string& proxy_domain,
                        int proxy_port)
{
    CNode n = g.getCNode();
    if (!n.valid()) {
        return;
    }
    std::ostringstream oss;
    GumboNode * apNode = reinterpret_cast<GumboNode*>(n.get());

    CQueryUtil::CIndenter indent(get_gqnode_indent());

    // NOTE 如果开启了 rewrite_original，则相当于一开始，就使用pre-mode——不添加缩进
    // ，不添加额外的空格；并且保留空白符号
    gumbo_rewrite_outterHtml(oss, apNode, indent, output_dir, rs_mgr, request_header,
                             proxy_domain, proxy_port, detail::html::get_rewrite_original());
    std::string content(oss.str());
    int ec = ::write(fd, content.c_str(), content.size());
    if (ec == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    COLOG_DEBUG(fd, content.size(), SSS_VALUE_MSG(ec));
}

void         set_gqnode_indent(const std::string& ind)
{
    size_t space_cnt = 0;
    while (space_cnt < ind.size() && std::isspace(ind[space_cnt])) {
        ++space_cnt;
    }
    get_gqnode_indent().assign(ind, 0, space_cnt);
}

std::string& get_gqnode_indent()
{
    static std::string indent = " ";
    return indent;
}

void         set_rewrite_original(bool o)
{
    get_rewrite_original() = o;
}

bool&        get_rewrite_original()
{
    static bool rewrite_original = false;
    return rewrite_original;
}

} // namespace html
} // namespace detail
} // namespace varlisp
