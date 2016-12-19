#include "html.hpp"

#include <gumbo_query/QueryUtil.h>
#include <gumbo_query/DocType.h>

#include <sss/string_view.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/utlstring.hpp>

#include <ss1x/asio/headers.hpp>
#include <ss1x/asio/utility.hpp>
#include <ss1x/uuid/sha1.hpp>

#include "http.hpp"

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

inline htmlEntityEscape_t htmlEntityEscape(sss::string_view s)
{
    return htmlEntityEscape_t{s};
}

inline std::ostream& operator << (std::ostream&o, const htmlEntityEscape_t& h)
{
    h.print(o);
    return o;
}

std::string getResourceAuto(const std::string& output_dir, const std::string& url,
                            resource_manager_t& rs_mgr)
{
    std::string max_content;
    ss1x::http::Headers headers;

    detail::http::downloadUrl(url, max_content, headers, ss1x::asio::redirectHttpGet);

    if (headers.status_code != 200) {
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
        fnameSuffix = "." + sss::path::basename(headers["Content-Type"]);
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

void gumbo_rewrite_outterHtml(std::ostream& o, GumboNode* apNode,
                              CQueryUtil::CIndenter& ind,
                              const std::string& output_dir,
                              resource_manager_t& rs_mgr)
{
    switch (apNode->type)
    {
        case GUMBO_NODE_TEXT:
            if (CQueryUtil::childNum(apNode->parent) >= 2) {
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
                bool is_self_close = CQueryUtil::isSelfCloseTag(apNode);
                bool is_neat_print = (CQueryUtil::childNum(apNode) == 1 &&
                                      CQueryUtil::childNum(CQueryUtil::nthChild(apNode, 0)) == 0);

                const std::string tagName = CQueryUtil::tagName(apNode);
                // COLOG_ERROR(tagName);
                o << ind << "<" << tagName;
                for (size_t i = 0; i < CQueryUtil::attrNum(apNode); ++i) {
                    std::string attrName = CQueryUtil::nthAttr(apNode, i)->name;
                    if (attrName == "href" || attrName == "src") {
                        // a.href 外链
                        // img.src 内部资源
                        // COLOG_ERROR(CQueryUtil::nthAttr(apNode, i)->value);
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
                        // TODO
                        // <link rel="stylesheet" href="http://static.blog.csdn.net/css/blog_code.css"/>
                        // <script src="http://static.blog.csdn.net/scripts/jquery.js" type="text/javascript"></script>
                        // css的问题在于，部分服务器，使用了css"融合"技术，即同一个链接，虽然获取到的是一个css文件，
                        // 但实际上是由后端，将多个css组合后，再发给浏览器的。此时，css的文件名，很难看……
                        // 另外一种风格，就是 http://static.blog.csdn.net/skin/default/css/style.css?v=1.1
                        // 这种，附带版本号的东西了。
                        //
                    }
                    if (tagName == "img" && attrName == "src") {
                        std::string url = CQueryUtil::nthAttr(apNode, i)->value;
                        if (rs_mgr.find(url) == rs_mgr.end()) {
                            std::string output_path = getResourceAuto(output_dir, url, rs_mgr);
                        }
                        if (rs_mgr[url].fsize && rs_mgr[url].is_ok()) {
                            o << " " << attrName << "=\""
                                << htmlEntityEscape(sss::path::basename(rs_mgr[url].path)) << "\"";
                        }
                        else {
                            o << " " << attrName << "=\""
                                << htmlEntityEscape(url) << "\"";
                        }
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
                        if (CQueryUtil::isGumboType(CQueryUtil::nthChild(apNode, i), GUMBO_NODE_WHITESPACE)) {
                            continue;
                        }
                        if (is_neat_print) {
                            CQueryUtil::CIndenter indInner(ind.get().c_str());
                            gumbo_rewrite_outterHtml(
                                o, CQueryUtil::nthChild(apNode, i), indInner,
                                output_dir, rs_mgr);
                        }
                        else {
                            gumbo_rewrite_outterHtml(
                                o, CQueryUtil::nthChild(apNode, i), ind,
                                output_dir, rs_mgr);
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
            o << ind << "<!CDATA[" << CQueryUtil::getText(apNode) << "]]>";
            break;

        case GUMBO_NODE_COMMENT:
            o << ind << "<!--" << CQueryUtil::getText(apNode) << "-->";
            break;

        case GUMBO_NODE_TEMPLATE:
            o << ind << "<!-- GUMBO_NODE_TEMPLATE not implement -->";
            break;

        case GUMBO_NODE_DOCUMENT:
            o << ind << CDocType(&apNode->v.document) << std::endl;
            for (size_t i = 0; i < CQueryUtil::childNum(apNode); i++)
            {
                gumbo_rewrite_outterHtml(o, CQueryUtil::nthChild(apNode, i),
                                         ind, output_dir, rs_mgr);
            }
            break;

        case GUMBO_NODE_WHITESPACE:
            //o << ind << " ";
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
                        const std::string& output_dir, resource_manager_t& rs_mgr)
{
    CNode n = g.getCNode();
    if (!n.valid()) {
        return;
    }
    std::ostringstream oss;
    GumboNode * apNode = reinterpret_cast<GumboNode*>(n.get());

    CQueryUtil::CIndenter indent(" ");
    gumbo_rewrite_outterHtml(oss, apNode, indent, output_dir, rs_mgr);
    std::string content(oss.str());
    int ec = ::write(fd, content.c_str(), content.size());
    if (ec == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    COLOG_ERROR(fd, content.size(), SSS_VALUE_MSG(ec));
}

void         set_gpnode_indent(const std::string& ind)
{
    size_t space_cnt = 0;
    while (space_cnt < ind.size() && std::isspace(ind[space_cnt])) {
        ++space_cnt;
    }
    get_gpnode_indent().assign(ind, 0, space_cnt);
}

std::string& get_gpnode_indent()
{
    static std::string indent = " ";
    return indent;
}

} // namespace html
} // namespace detail
} // namespace varlisp
