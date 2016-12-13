#include "html.hpp"

#include <gumbo_query/QueryUtil.h>
#include <gumbo_query/DocType.h>

#include <sss/string_view.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include <ss1x/asio/headers.hpp>
#include <ss1x/asio/utility.hpp>

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
        for (auto ch : *this)
        {
            switch (ch) {
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
                    o << ch;
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
    std::string raw_url;
    auto question_mark_pos = raw_url.find('?');
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

    std::string fname = sss::path::no_suffix(sss::path::basename(raw_url));
    sss::path::append(output_path, fname + fnameSuffix);

    if (sss::path::file_exists(output_path) == sss::PATH_TO_FILE) {
        rs_mgr[url] = {output_path, max_content.size(), fs_EXIST};
        return output_path;
    }
    std::ofstream ofs(output_path);
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

} // namespace html
} // namespace detail
} // namespace varlisp
