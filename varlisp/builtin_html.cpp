#include <ss1x/asio/utility.hpp>
#include <sss/utlstring.hpp>

#include "object.hpp"
#include "list.hpp"
#include "builtin_helper.hpp"

namespace varlisp {

/**
 * @brief
 *      (gumbo "<html>") -> gumboNode
 *      (gumbo "<html>" "query-string") -> '(gumboNode)
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
        varlisp::getTypedValue<string_t>(env, args.head, content);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires 1st argument as content to parsing)");
    }
    Object query;
    const string_t* p_query = 0;

    if (args.length() >= 2) {
        varlisp::getTypedValue<string_t>(env, args.tail[0].head, query);
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
            varlisp::List* p_ret_nodes = &ret_nodes;
            for (auto& item : vec) {
                p_ret_nodes = p_ret_nodes->next_slot();
                p_ret_nodes->head = item;
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
        varlisp::getTypedValue<gumboNode>(env, args.head, gNode);
    if (!p_node) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumboNode as 1st argument to querying)");
    }
    Object query;
    const string_t* p_query =
        varlisp::getTypedValue<string_t>(env, args.tail[0].head, query);
    if (!p_query) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires query string as 2nd argument)");
    }

    varlisp::List ret_nodes = varlisp::List::makeSQuoteList();
    if (p_node->valid() && p_query && p_query->length()) {
        std::vector<gumboNode> vec = p_node->find(p_query->to_string());
        varlisp::List* p_ret_nodes = &ret_nodes;
        for (auto& item : vec) {
            p_ret_nodes = p_ret_nodes->next_slot();
            p_ret_nodes->head = item;
        }
    }

    return ret_nodes;
}

/**
 * @brief
 *      (gqnode:attr gumboNode "attrib-name") -> "attrib-value" | nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_attr(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode:attr";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    Object attrib_name;
    const string_t* p_attrib_name = varlisp::getTypedValue<string_t>(
        env, args.tail[0].head, attrib_name);
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

/**
 * @brief
 *      (gqnode:hasAttr gumboNode "attrib-name") -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_hasAttr(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode:hasAttr";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    Object attrib_name;
    const string_t* p_attrib_name = varlisp::getTypedValue<string_t>(
        env, args.tail[0].head, attrib_name);
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
            varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
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

/**
 * @brief
 *      (gqnode:valid gumboNode) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_valid(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode:valid";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    return p_gqnode->valid();
}

/**
 * @brief
 *      (gqnode:isText gumboNode) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_isText(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode:isText";
    Object gqnode;
    const gumboNode* p_gqnode =
        varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    if (!p_gqnode) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires gumbo-query-node as 1st argument)");
    }
    if (p_gqnode->valid()) {
        return p_gqnode->isText();
    }
    return Object{Nill{}};
}


/**
 * @brief
 *      (gqnode:text gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_text(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "gqnode:text";
    return gumboNodeMethodWrapper(funcName, &gumboNode::text)(env, args);
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->text()));
    // }
    // return Object{Nill{}};
}

/**
 * @brief
 *      (gqnode:textNeat gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_textNeat(varlisp::Environment& env,
                            const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode:textNeat", &gumboNode::textNeat)(env, args);
    // const char* funcName = "gqnode:textNeat";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->textNeat()));
    // }
    // return Object{Nill{}};
}

/**
 * @brief
 *      (gqnode:ownText gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_ownText(varlisp::Environment& env, const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode:ownText", &gumboNode::onwText)(env, args);
    // const char* funcName = "gqnode:ownText";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->onwText()));
    // }
    // return Object{Nill{}};
}

/**
 * @brief
 *      (gqnode:tag gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_tag(varlisp::Environment& env, const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode:tag", &gumboNode::tag)(env, args);
    // const char* funcName = "gqnode:tag";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->tag()));
    // }
    // return Object{Nill{}};
}

/**
 * @brief
 *      (gqnode:innerHtml gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_innerHtml(varlisp::Environment& env,
                             const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode:innerHtml", &gumboNode::innerHtml)(env, args);
    // const char* funcName = "gqnode:innerHtml";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->innerHtml()));
    // }
    // return Object{Nill{}};
}

/**
 * @brief
 *      (gqnode:outerHtml gumboNode) -> "text"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gqnode_outerHtml(varlisp::Environment& env,
                             const varlisp::List& args)
{
    return gumboNodeMethodWrapper("gqnode:outerHtml", &gumboNode::outerHtml)(env, args);
    // const char* funcName = "gqnode:outerHtml";
    // Object gqnode;
    // const gumboNode* p_gqnode =
    //     varlisp::getTypedValue<gumboNode>(env, args.head, gqnode);
    // if (!p_gqnode) {
    //     SSS_POSITION_THROW(std::runtime_error, "(", funcName,
    //                       ": requires gumbo-query-node as 1st argument)");
    // }
    // if (p_gqnode->valid()) {
    //     return string_t(std::move(p_gqnode->outerHtml()));
    // }
    // return Object{Nill{}};
}

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
        varlisp::getTypedValue<string_t>(env, args.head, content);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": requires content to parsing)");
    }
    Object query;
    const string_t* p_query =
        varlisp::getTypedValue<string_t>(env, args.tail[0].head, query);
    if (!p_query) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": requires query string)");
    }
    std::ostringstream oss;
    ss1x::util::html::queryText(oss, p_content->to_string(), p_query->to_string());

    return string_t(std::move(oss.str()));
}

}  // namespace varlisp
