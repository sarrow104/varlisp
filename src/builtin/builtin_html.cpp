#include <array>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <gq/QueryUtil.h>
#include <gq/DocType.h>

#include <sss/utlstring.hpp>
#include <sss/path.hpp>
#include <sss/debug/value_msg.hpp>

#include <ss1x/asio/utility.hpp>
#include <ss1x/asio/headers.hpp>

#include "../object.hpp"
#include "../list.hpp"
#include "../builtin_helper.hpp"
#include "../raw_stream_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/html.hpp"
#include "../detail/http.hpp"
#include "../detail/file.hpp"
#include "../detail/io.hpp"

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
    std::array<Object, 2> objs;
    const string_t* p_content =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_query = 0;

    if (args.length() >= 2) {
        p_query =
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
    }
    if (p_content->empty()) {
        return Object{Nill{}};
    }
    gumboNode doc{p_content->gen_shared()};
    if (doc.valid()) {
        if (p_query && p_query->length()) {
            std::vector<gumboNode> vec = doc.find(*p_query->gen_shared());
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
               ";\t p:first 第一个p元素\n"
               ";\t tr:even 所有偶数<tr>元素\n"
               ";\t ul>li:gt(3) ul中第四个以及以后的li元素(index从0开始)\n"
               ";\t input:not(:empty) 所有不为空的input元素\n"
               ";\t :header 所有标题元素\n"
               ";\t :contains('W3School') 包含指定字符串的所有元素\n"
               ";\t th,td,.intro 并列规则，满足其一的所有元素\n"
               ";\t [href] 含有href属性的所有元素\n"
               ";\t [href!='#'] 拥有不等于'#'的href属性的所有元素\n"

               "; dsl 语法规则\n"
               "; http://www.w3school.com.cn/jquery/jquery_ref_selectors.asp\n"
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
    std::array<Object, 2> objs;
    const gumboNode* p_node =
        requireTypedValue<gumboNode>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_query =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
    if (p_node->valid() && p_query && p_query->length()) {
        std::vector<gumboNode> vec = p_node->find(*p_query->gen_shared());
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
        requireTypedValue<gumboNode>(env, args.nth(0), gNode, funcName, 0, DEBUG_INFO);

    varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
    std::vector<gumboNode> vec = p_node->children();
    auto back_it = detail::list_back_inserter<Object>(ret_nodes);
    for (auto& item : vec) {
        *back_it++ = item;
    }

    return ret_nodes;
}

REGIST_BUILTIN("gqnode-indent", 0, 1, eval_gqnode_indent,
               "; gqnode-indent 获取或者设定gqnode在打印的时候的文本缩进设定;\n"
               "; 传入、返回的的都是字符串；非空白符号的设定，会被忽略\n"
               "(gqnode-indent) -> \"current-indent\"\n"
               "(gqnode-indent \"new-indent\") -> \"new-accept-indent\"");

Object eval_gqnode_indent(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode-indent";
    if (args.size()) {
        Object tmp;
        const auto * p_indent =requireTypedValue<varlisp::string_t>(env, args.nth(0), tmp, funcName, 0, DEBUG_INFO);
        detail::html::set_gqnode_indent(*p_indent->gen_shared());
    }
    return string_t(detail::html::get_gqnode_indent());
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
    std::array<Object, 2> objs;
    const gumboNode* p_gqnode =
        requireTypedValue<gumboNode>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_attrib_name =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    if (p_gqnode->valid()) {
        std::string attribute = p_gqnode->attribute(*p_attrib_name->gen_shared());
        // NOTE empty string is ok!
        return string_t{std::move(attribute)};
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

    std::array<Object, 2> objs;
    const gumboNode* p_gqnode =
        requireTypedValue<gumboNode>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_attrib_name =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    if (p_gqnode->valid()) {
        return p_gqnode->hasAttr(*p_attrib_name->gen_shared());
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
            requireTypedValue<gumboNode>(env, args.nth(0), gqnode, m_funcName, 0, DEBUG_INFO);
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
        requireTypedValue<gumboNode>(env, args.nth(0), gqnode, funcName, 0, DEBUG_INFO);
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
        requireTypedValue<gumboNode>(env, args.nth(0), gqnode, funcName, 0, DEBUG_INFO);

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
    std::array<Object, 2> objs;
    const string_t* p_content =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t* p_query =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    std::ostringstream oss;
    ss1x::util::html::queryText(oss, *p_content->gen_shared(), *p_query->gen_shared());

    return string_t(std::move(oss.str()));
}

REGIST_BUILTIN("gumbo-original-rewrite", 0, 1, eval_gumbo_original_rewrite,
               "; gumbo-original-rewrite 是否按原始格式，重写html文本\n"
               "; 获取该设定值，或者修改该值；默认否(#f)\n"
               "(gumbo-original-rewrite) -> boolean\n"
               "(gumbo-original-rewrite boolean) -> boolean\n");

Object eval_gumbo_original_rewrite(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "gumbo-original-rewrite";
    if (args.length()) {
        std::array<Object, 1> objs;
        const auto* p_bool =
            requireTypedValue<bool>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
        detail::html::set_rewrite_original(*p_bool);
    }

    return detail::html::get_rewrite_original();
}

REGIST_BUILTIN("gumbo-rewrite", 2,  -1,  eval_gumbo_rewrite,
               "gumbo-rewrite 重写gumbo-node到指定文件描述符中\n"
               "注意，链接等会被改写\n"
               "文件名，如何确定？\n"
               "(gumbo-rewrite int-fd '(gq-node) "") -> nil\n"
               "(gumbo-rewrite int-fd {request_header} '(gq-node) "") -> nil"
               "(gumbo-rewrite [int-fd proxy-domain proxy-port] '(gq-node) "") -> nil\n"
               "(gumbo-rewrite [int-fd proxy-domain proxy-port] {request_header} '(gq-node) "") -> nil"
               );

Object eval_gumbo_rewrite(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "gumbo-gumbo-rewrite";
    std::array<Object, 2> objs;
    std::array<Object, 3> proxy_tmp;
    std::string proxy_domain;
    int64_t proxy_port = 0;

    int64_t fd = -1;
    if (auto * p_fd = varlisp::getTypedValue<int64_t>(env, args.nth(0), objs[0])) {
        fd = *p_fd;
    }
    else if (auto * p_list = varlisp::getQuotedList(env, args.nth(0), objs[0])) {
        fd = *requireTypedValue<int64_t>(
            env, p_list->nth(0), proxy_tmp[0],
            "(gumbo-rewrite [fd proxy-domain proxy-port] ...)", 0, DEBUG_INFO);

        proxy_domain = *requireTypedValue<varlisp::string_t>(
                           env, p_list->nth(1), proxy_tmp[1],
                           "(gumbo-rewrite [fd proxy-domain proxy-port] ...)",
                           1, DEBUG_INFO)
                           ->gen_shared();

        proxy_port = *requireTypedValue<int64_t>(
            env, p_list->nth(2), proxy_tmp[2],
            "(gumbo-rewrite [fd proxy-domain proxy-port] ...)", 2, DEBUG_INFO);
    }
    else {
        SSS_POSITION_THROW(
            std::runtime_error, "(", funcName,
            ": 1st argument must be fd or '(fd proxy_domain proxy_port) list");
    }

    std::string output_dir = sss::path::dirname(detail::file::get_fname_from_fd(fd));
    COLOG_DEBUG(SSS_VALUE_MSG(output_dir));

    ss1x::http::Headers request_header;

    // NOTE html正文其他部分，由调用本方法的过程来完成！这里，最多还需要考虑的是
    // ，1. 输出路径；2. 输出的时候，初始缩进；3. 枚举子节点的深度，仅文本？(信
    // 息过滤)
    // sss::string_view headHtml = "<html>";
    // sss::string_view tailHtml = "</html>";

    // 在重写的时候，如何处理重复的url？
    // 需要建立一个url与本地path的对应关系；同时，需要备注上下载状态；

    detail::html::resource_manager_t rs_mgr;
    for (size_t i = 1; i < args.length(); ++i) {
        auto& secondRef = varlisp::getAtomicValue(env, args.nth(i), objs[1]);
        if (i == 1) {
            if (auto * p_request_header = boost::get<varlisp::Environment>(&secondRef)) {
                varlisp::detail::http::Environment2ss1x_header(
                    request_header, env, *p_request_header);
                continue;
            }
        }

        Object gpNodeList;
        Object gpNodeObj;
        if (auto * p_list = varlisp::getQuotedList(env, secondRef, gpNodeList)) {
            for (auto it = p_list->begin(); it != p_list->end(); ++it) {
                auto * p_gp = varlisp::getTypedValue<gumboNode>(env, *it, gpNodeObj);
                if (!p_gp) {
                    std::ostringstream oss;
                    boost::apply_visitor(raw_stream_visitor(oss, env), *it);
                    detail::writestring(fd, oss.str());
                }
                else {
                    detail::html::gumbo_rewrite_impl(fd, *p_gp, output_dir,
                                                     rs_mgr, request_header,
                                                     proxy_domain, proxy_port);
                }
            }
        }
        else if (auto * p_gp = varlisp::getTypedValue<gumboNode>(env, secondRef, gpNodeObj)) {
            detail::html::gumbo_rewrite_impl(fd, *p_gp, output_dir, rs_mgr,
                                             request_header,
                                             proxy_domain, proxy_port);
        }
        else {
            std::ostringstream oss;
            boost::apply_visitor(raw_stream_visitor(oss, env), secondRef);
            detail::writestring(fd, oss.str());
        }
    }
    for (const auto kv : rs_mgr) {
        if (!kv.second.is_ok()) {
            COLOG_ERROR(sss::raw_string(kv.first), kv.second);
        }
    }

    return Nill{};
}

}  // namespace varlisp
