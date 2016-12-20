#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../helpmsg_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("help", 1, 1, eval_help, "(help symbol) -> nil");

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
    std::cout << boost::apply_visitor(helpmsg_visitor(env), detail::car(args))
              << std::endl;
    return varlisp::Nill{};
}

REGIST_BUILTIN("get-help", 1, 1, eval_get_help, "(get-help symbol) -> string");

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
    return boost::apply_visitor(helpmsg_visitor(env), detail::car(args));
}

} // namespace varlisp
