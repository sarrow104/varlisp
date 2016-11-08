#include "eval_visitor.hpp"
#include "object.hpp"

#include <ss1x/asio/utility.hpp>
#include <sss/utlstring.hpp>

namespace varlisp {
Object eval_gumbo_query(varlisp::Environment& env, const varlisp::List& args)
{
    Object content = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string* p_content = boost::get<std::string>(&content);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "gumbo-query requies content to parsing");
    }
    Object query = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string* p_query = boost::get<std::string>(&query);
    if (!p_query) {
        SSS_POSTION_THROW(std::runtime_error,
                          "gumbo-query requies query string");
    }
    std::ostringstream oss;
    ss1x::util::html::queryText(oss, *p_content, *p_query);

    return oss.str();
}

}  // namespace varlisp
