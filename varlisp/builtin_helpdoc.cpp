#include "object.hpp"
#include "builtin_helper.hpp"
#include "helpmsg_visitor.hpp"

namespace varlisp {

/**
 * @brief (help symbol) -> nil
 *    print help msg for symbol
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_help(varlisp::Environment& env, const varlisp::List& args)
{
    std::cout << boost::apply_visitor(helpmsg_visitor(env), args.head) << std::endl;
    return varlisp::Nill{};
}

/**
 * @brief (get-help symbol) -> string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return help-msg string for @symbol
 */
Object eval_get_help(varlisp::Environment& env, const varlisp::List& args)
{
    return boost::apply_visitor(helpmsg_visitor(env), args.head);
}

} // namespace varlisp
