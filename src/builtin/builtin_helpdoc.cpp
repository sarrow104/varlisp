#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../helpmsg_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include <sss/path.hpp>
#include <fstream>

namespace {

void simple_help(std::ostream& out) {
    std::string helpPath
        = sss::path::append_copy(
            sss::path::dirname(sss::path::getbin()),
            "help.md");
    std::ifstream ifs(helpPath, std::ios_base::in);
    out << ifs.rdbuf();
}

} // namespace

namespace varlisp {


REGIST_BUILTIN("help", 0, 1, eval_help,
               "打印一个简单的帮助说明\n"
               "(help) -> nil\n"
               "获取打印某对象的帮助文字\n"
               "(help symbol) -> nil");

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
    if (args.length() != 0U) {
        std::cout
            << boost::apply_visitor(helpmsg_visitor(env), detail::car(args))
            << std::endl;
    } else {
        ::simple_help(std::cout);
    }
    return varlisp::Nill{};
}

REGIST_BUILTIN("get-help", 1, 1, eval_get_help,
               "获取某对象的帮助文字\n"
               "(get-help symbol) -> string");

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
