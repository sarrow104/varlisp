#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <gumbo_query/QueryUtil.h>
#include <gumbo_query/DocType.h>

#include <sss/utlstring.hpp>
#include <sss/path.hpp>
#include <sss/debug/value_msg.hpp>

#include <ss1x/asio/utility.hpp>
#include <ss1x/asio/headers.hpp>

#include "object.hpp"
#include "list.hpp"
#include "builtin_helper.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"
#include "detail/list_iterator.hpp"
#include "detail/http.hpp"

namespace varlisp {

REGIST_BUILTIN("gumbo", 1, 2, eval_gumbo,
               "; gumbo 从 utf8 html 正文，生成gumbo-document 文档对象\n"
               "(gumbo \"<html>\") -> gumboNode\n"
               "(gumbo \"<html>\" \"query-string\") -> '(gumboNode)");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gumbo(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gumbo";
    Object content;
    const string_t* p_content =
        varlisp::getTypedValue<string_t>(env, detail::car(args), content);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires 1st argument as content to parsing)");
    }
    Object query;
    const string_t* p_query = 0;

    if (args.length() >= 2) {
        varlisp::getTypedValue<string_t>(env, detail::cadr(args), query);
        if (!p_query) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                              "： requires 2nd argument as query string)");
        }
    }
    if (p_content->empty()) {
        return Object{Nill{}};
    }
    gumboNode doc{p_content->to_string()};
    if (doc.valid()) {
        if (p_query && p_query->length()) {
            std::vector<gumboNode> vec = doc.find(p_query->to_string());
            varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
            auto back_it = detail::list_back_inserter<Object>(ret_nodes);
            for (auto& item : vec) {
                *back_it++ = item;
            }
            return ret_nodes;
        }
        else {
            return doc;
        }
    }
    else {
        return Object{Nill{}};
    }
}

REGIST_BUILTIN("gumbo-query", 2, 2, eval_gumbo_query,
               "; gumbo-query 类似jquery语法的 gumboNode 检索\n"
               "; 范例\n"
               ";\t h1 a.special\n"
               ";\t span[id=\"that's\"]\n"
               "; dsl 语法规则\n"
               ";\t SelectorGroup\n"
               ";\t  -> Selector ( ',' Selector ) *\n"
               ";\t \n"
               ";\t Selector\n"
               ";\t  -> SimpleSelectorSequence ( ' ' SimpleSelectorSequence ) *  # 直系子孙\n"
               ";\t  -> SimpleSelectorSequence ( '>' SimpleSelectorSequence ) *  # 儿子\n"
               ";\t  -> SimpleSelectorSequence ( '+' SimpleSelectorSequence ) *  # 相邻\n"
               ";\t  -> SimpleSelectorSequence ( '~' SimpleSelectorSequence ) *  # 同上\n"
               ";\t \n"
               ";\t SimpleSelectorSequence\n"
               ";\t   -> ('*'|TypeSelector)? IDSelector           // begin with '#'\n"
               ";\t   -> ('*'|TypeSelector)? ClassSelector        // begin with '.'\n"
               ";\t   -> ('*'|TypeSelector)? AttributeSelector    // begin with '['\n"
               ";\t   -> ('*'|TypeSelector)? PseudoclassSelector  // begin with ':'\n"
               ";\t \n"
               ";\t Nth\n"
               ";\t  -> ('+'|'-')? (integer|'n'|\"odd\"|\"even\")\n"
               ";\t \n"
               ";\t Integer\n"
               ";\t   -> [0-9]+\n"
               ";\t \n"
               ";\t PseudoclassSelector\n"
               ";\t  -> ':' (\"not\"|\"has\"|\"haschild\") '(' SelectorGroup ')'\n"
               ";\t  -> ':' (\"contains\"|\"containsown\") '(' (String | Identifier) ')' // 貌似是case sensitive\n"
               ";\t  -> ':' (\"matches\"|\"matchesown\") // TODO not support!\n"
               ";\t  -> ':' (\"nth-child\"|\"nth-last-child\"|\"nth-of-type\"|\"nth-last-of-type\") '(' Nth ')'\n"
               ";\t  -> ':' \"first-child\"\n"
               ";\t  -> ':' \"last-child\"\n"
               ";\t  -> ':' \"first-of-type\"\n"
               ";\t  -> ':' \"last-of-type\"\n"
               ";\t  -> ':' \"only-child\"\n"
               ";\t  -> ':' \"only-of-type\"\n"
               ";\t  -> ':' \"empty\"\n"
               ";\t  -> ':' \"lang\" '(' The-escaped-value ')' // TODO\n"
               ";\t \n"
               ";\t AttributeSelector\n"
               ";\t  -> '[' Identifier ']'\n"
               ";\t  -> '[' Identifier '=' (String|Identifier)']'\n"
               ";\t  -> '[' Identifier (\"~=\"|\"!=\" |\"^=\" |\"$=\" |\"*=\") (String|Identifier) ']'\n"
               ";\t  -> '[' Identifier '#' '=' ... // TODO support regex\n"
               ";\t \n"
               ";\t ClassSelector\n"
               ";\t  -> '.' Identifier\n"
               ";\t \n"
               ";\t IDSelector\n"
               ";\t  -> '#' Name\n"
               ";\t \n"
               ";\t TypeSelector\n"
               ";\t  -> Identifier //check by \"gumbo_tag_enum\"\n"
               "(gumbo-query gumboNode \"selector-string\")\n"
               " -> '(gumboNodes)");

/**
 * @brief
 *      (gumbo-query gumboNode "selector-string")
 *           -> '(gumboNodes)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gumbo_query(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gumbo-query";
    Object gNode;
    const gumboNode* p_node =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gNode);
    if (!p_node) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumboNode as 1st argument to querying)");
    }
    Object query;
    const string_t* p_query =
        varlisp::getTypedValue<string_t>(env, detail::cadr(args), query);
    if (!p_query) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires query string as 2nd argument)");
    }

    varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
    if (p_node->valid() && p_query && p_query->length()) {
        std::vector<gumboNode> vec = p_node->find(p_query->to_string());
        auto back_it = detail::list_back_inserter<Object>(ret_nodes);
        for (auto& item : vec) {
            *back_it++ = item;
        }
    }

    return ret_nodes;
}

REGIST_BUILTIN("gumbo-children", 1, 1, eval_gumbo_children,
               "; gumbo-children 枚举子节点；"
               "; 相当于更快的(gumbo-query gnode \"*\")\n"
               "(gumbo-children gumboNode) -> '(gumboNodes)");

Object eval_gumbo_children(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gumbo-query";
    Object gNode;
    const gumboNode* p_node =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gNode);
    if (!p_node) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumboNode as 1st argument to querying)");
    }

    varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
    std::vector<gumboNode> vec = p_node->children();
    auto back_it = detail::list_back_inserter<Object>(ret_nodes);
    for (auto& item : vec) {
        *back_it++ = item;
    }

    return ret_nodes;
}

REGIST_BUILTIN(
    "gqnode-attr", 2, 2, eval_gqnode_attr,
    "(gqnode-attr gumboNode \"attrib-name\") -> \"attrib-value\" | nil");

/**
 * @brief
 *      (gqnode-attr gumboNode "attrib-name") -> "attrib-value" | nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_attr(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-attr";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    Object attrib_name;
    const string_t* p_attrib_name = varlisp::getTypedValue<string_t>(
        env, detail::cadr(args), attrib_name);
    if (!p_attrib_name) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    if (p_gqnode->valid()) {
        std::string attribute = p_gqnode->attribute(p_attrib_name->to_string());
        if (!attribute.empty()) {
            return string_t{std::move(attribute)};
        }
    }
    return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-hasAttr", 2, 2, eval_gqnode_hasAttr,
               "(gqnode-hasAttr gumboNode \"attrib-name\") -> boolean");

/**
 * @brief
 *      (gqnode-hasAttr gumboNode "attrib-name") -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_hasAttr(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-hasAttr";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    Object attrib_name;
    const string_t* p_attrib_name = varlisp::getTypedValue<string_t>(
        env, detail::cadr(args), attrib_name);
    if (!p_attrib_name) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    if (p_gqnode->valid()) {
        return p_gqnode->hasAttr(p_attrib_name->to_string());
    }
    return Object{Nill{}};
}

typedef std::string (gumboNode::*gbNodeMethod_t)() const;
struct gumboNodeMethodWrapper {
    gumboNodeMethodWrapper(const char* funcName, gbNodeMethod_t m)
        : m_funcName(funcName), m_method(m)
    {
    }

public:
    Object operator()(varlisp::Environment& env, const varlisp::List& args)
    {
        Object gqnode;
        const gumboNode* p_gqnode =
            varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
        if (!p_gqnode) {
            SSS_POSITION_THROW(std::runtime_error, "(", m_funcName,
                               ": requires gumbo-query-node as 1st argument)");
        }
        if (p_gqnode->valid()) {
            return string_t(std::move((p_gqnode->*m_method)()));
        }
        return Object{Nill{}};
    }

private:
    const char* m_funcName;
    gbNodeMethod_t m_method;
};

REGIST_BUILTIN("gqnode-valid", 1, 1, eval_gqnode_valid,
               "(gqnode-valid gumboNode) -> boolean");

/**
 * @brief
 *      (gqnode-valid gumboNode) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_valid(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-valid";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    return p_gqnode->valid();
}

REGIST_BUILTIN("gqnode-isText",   1,  1,  eval_gqnode_isText,
               "(gqnode-isText gumboNode) -> boolean");

/**
 * @brief
 *      (gqnode-isText gumboNode) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_isText(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-isText";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    if (p_gqnode->valid()) {
        return p_gqnode->isText();
    }
    return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-text", 1, 1, eval_gqnode_text,
               "(gqnode-text gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-text gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_text(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-text";
    return gumboNodeMethodWrapper(funcName, &gumboNode::text)(env, args);
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->text()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-textNeat", 1, 1, eval_gqnode_textNeat,
               "(gqnode-textNeat gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-textNeat gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_textNeat(varlisp::Environment& env,
                            const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode-textNeat", &gumboNode::textNeat)(env, args);
    // const char* funcName = "gqnode-textNeat";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->textNeat()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-ownText", 1, 1, eval_gqnode_ownText,
               "(gqnode-ownText gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-ownText gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_ownText(varlisp::Environment& env, const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode-ownText", &gumboNode::onwText)(env, args);
    // const char* funcName = "gqnode-ownText";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->onwText()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-tag", 1, 1, eval_gqnode_tag,
               "(gqnode-tag gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-tag gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_tag(varlisp::Environment& env, const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode-tag", &gumboNode::tag)(env, args);
    // const char* funcName = "gqnode-tag";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->tag()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-innerHtml", 1, 1, eval_gqnode_innerHtml,
               "(gqnode-innerHtml gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-innerHtml gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_innerHtml(varlisp::Environment& env,
                             const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode-innerHtml", &gumboNode::innerHtml)(env, args);
    // const char* funcName = "gqnode-innerHtml";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->innerHtml()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gqnode-outerHtml", 1, 1, eval_gqnode_outerHtml,
               "(gqnode-outerHtml gumboNode) -> \"text\"");

/**
 * @brief
 *      (gqnode-outerHtml gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_outerHtml(varlisp::Environment& env,
                             const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode-outerHtml", &gumboNode::outerHtml)(env, args);
    // const char* funcName = "gqnode-outerHtml";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, detail::car(args), gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->outerHtml()));
    // }
    // return Object{Nill{}};
}

REGIST_BUILTIN("gumbo-query-text",2,  2,  eval_gumbo_query_text,
               "(gumbo-query-text \"<html>\" \"selector-string\") \"node->text\"");

// NOTE 如何处理多个node，然后需要串成一个s-list的需求？
// map-line ?

/**
 * @brief
 *      (gumbo-query-text "<html>" "selector-string")
 *
 * @param[in] env
 * @param[in] args
 *
 * @return "node->text"
 */
Object eval_gumbo_query_text(varlisp::Environment& env,
                             const varlisp::List& args)
{
    const char * funcName = "gumbo-query-text";
    Object content;
    const string_t* p_content =
        varlisp::getTypedValue<string_t>(env, detail::car(args), content);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": requires content to parsing)");
    }
    Object query;
    const string_t* p_query =
        varlisp::getTypedValue<string_t>(env, detail::cadr(args), query);
    if (!p_query) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": requires query string)");
    }
    std::ostringstream oss;
    ss1x::util::html::queryText(oss, p_content->to_string(), p_query->to_string());

    return string_t(std::move(oss.str()));
}

namespace detail {
std::string get_fname_from_fd(int fd)
{
    const size_t buf_size = 256;
    char s[buf_size];
    std::snprintf(s, buf_size - 1, "/proc/%d/fd/%d", getpid(), fd);
    std::string name;
    name.resize(buf_size);
    while (true) {
        int ec = readlink(s, const_cast<char*>(&name[0]), name.size() - 1);
        if (ec > 0) {
            name.resize(ec);
            break;
        }
        else if (ec == -1) {
            switch (errno) {
                case ENAMETOOLONG:
                    name.resize(name.size() * 2);
                    continue;

                default:
                    SSS_POSITION_THROW(std::runtime_error,
                                       std::strerror(errno));
                    break;
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "unkown readlink return value:", ec,
                               "; errno = ", errno, std::strerror(errno));
        }
    }
    return name;
}

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

std::string getResourceAuto(const std::string& output_dir, const std::string& url)
{
    std::string max_content;
    ss1x::http::Headers headers;

    detail::http::downloadUrl(url, max_content, headers, ss1x::asio::redirectHttpGet);
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
    std::ofstream ofs(output_path);
    ofs << max_content;

    COLOG_INFO(url, " -> ", output_path, "; ", max_content.size(), " bytes.");
    return output_path;
}

void gumbo_rewrite_outterHtml(std::ostream& o, GumboNode* apNode,
                              CQueryUtil::CIndenter& ind,
                              const std::string& output_dir)
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
                        std::string output_path = getResourceAuto(output_dir, url);
                        o << " " << attrName << "=\""
                            << htmlEntityEscape(sss::path::basename(output_path)) << "\"";
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
                            gumbo_rewrite_outterHtml(o, CQueryUtil::nthChild(apNode, i), indInner, output_dir);
                        }
                        else {
                            gumbo_rewrite_outterHtml(o, CQueryUtil::nthChild(apNode, i), ind, output_dir);
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
                gumbo_rewrite_outterHtml(o, CQueryUtil::nthChild(apNode, i), ind, output_dir);
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

void gumbo_rewrite_impl(int fd, const gumboNode& g, const std::string& output_dir)
{
    CNode n = g.getCNode();
    if (!n.valid()) {
        return;
    }
    std::ostringstream oss;
    GumboNode * apNode = reinterpret_cast<GumboNode*>(n.get());

    CQueryUtil::CIndenter indent(" ");
    gumbo_rewrite_outterHtml(oss, apNode, indent, output_dir);
    std::string content(std::move(oss.str()));
    int ec = ::write(fd, content.c_str(), content.size());
    if (ec == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    COLOG_ERROR(fd, content.size(), SSS_VALUE_MSG(ec));
}

} // namespace detail

REGIST_BUILTIN("gumbo-rewrite", 2,  -1,  eval_gumbo_rewrite,
               "gumbo-rewrite 重写gumbo-node到指定文件描述符中\n"
               "注意，链接等会被改写\n"
               "文件名，如何确定？\n"
               "(gumbo-gumbo-rewrite int-fd '(gq-node) "") -> nil");

Object eval_gumbo_rewrite(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "gumbo-gumbo-rewrite";
    Object fdObj;
    const int64_t * p_fd = varlisp::getTypedValue<int64_t>(env, detail::car(args), fdObj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st argument must be int fd)");
    }

    std::string output_dir = sss::path::dirname(detail::get_fname_from_fd(*p_fd));
    COLOG_ERROR(output_dir);

    // NOTE html正文其他部分，由调用本方法的过程来完成！这里，最多还需要考虑的是
    // ，1. 输出路径；2. 输出的时候，初始缩进；3. 枚举子节点的深度，仅文本？(信
    // 息过滤)
    // sss::string_view headHtml = "<html>";
    // sss::string_view tailHtml = "</html>";

    Object second;
    auto& secondRef = varlisp::getAtomicValue(env, detail::cadr(args), second);
    Object gpNodeList;
    Object gpNodeObj;
    if (auto * p_list = varlisp::getQuotedList(env, secondRef, gpNodeList)) {
        for (auto it = p_list->begin(); it != p_list->end(); ++it) {
            auto * p_gp = varlisp::getTypedValue<gumboNode>(env, *it, gpNodeObj);
            if (!p_gp) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "(", funcName, ": 2st argument must be construct by gpnode)");
            }
            detail::gumbo_rewrite_impl(*p_fd, *p_gp, output_dir);
        }
    }
    else if (auto * p_gp = varlisp::getTypedValue<gumboNode>(env, secondRef, gpNodeObj)) {
        detail::gumbo_rewrite_impl(*p_fd, *p_gp, output_dir);
    }
    else {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": cannot rewirte ", secondRef, " to ", *p_fd, ")");
    }

    return Nill{};
}

}  // namespace varlisp
