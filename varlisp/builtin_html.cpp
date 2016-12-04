#include <ss1x/asio/utility.hpp>
#include <sss/utlstring.hpp>

#include "object.hpp"
#include "list.hpp"
#include "builtin_helper.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"
#include "detail/list_iterator.hpp"

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
    gumboNodeMethodWrapper(const char* funcName, gbNodeMethod_t m) : m_funcName(funcName), m_method(m)
    {}
public:
    Object operator() (varlisp::Environment& env, const varlisp::List& args) {
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
    const char * m_funcName;
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

}  // namespace varlisp
