#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"
#include "../environment.hpp"
#include "../interpreter.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("quit", 0, 0, eval_quit, "(quit) -> #t");

/**
 * @brief (quit)    ->  #t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_quit(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;
    Interpreter::get_instance().set_status(Interpreter::status_QUIT);
    return true;
}

REGIST_BUILTIN("it-debug", 1, 1, eval_it_debug, "(it-debug #t|#f) -> nil");

/**
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_it_debug(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "it-debug";
    Object status;
    const bool* p_status =
        requireTypedValue<bool>(env, args.nth(0), status, funcName, 0, DEBUG_INFO);

    auto ll = sss::colog::get_log_levels();
    sss::colog::log_level next_ll = ll;
    if (*p_status) {
        next_ll = (next_ll | sss::colog::ll_DEBUG) & sss::colog::ll_MASK;
    }
    else {
        next_ll = next_ll & ~sss::colog::ll_DEBUG & sss::colog::ll_MASK;
    }

    sss::colog::set_log_levels(next_ll);
    return *p_status;
}

REGIST_BUILTIN("colog-style", 0, -1, eval_colog_format,
               "(colog-format CL_ELEMENT) -> current-format-mask");

Object eval_colog_format(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "colog-style";
    if (args.length() != 0U) {
        std::array<Object, 1> objs;
        sss::colog::log_style next_style = sss::colog::ls_NONE;
        for (size_t i = 0; i != args.length(); ++i) {
            next_style = next_style | sss::colog::log_style(*requireTypedValue<int64_t>(env, args.nth(i), objs[0], funcName, i, DEBUG_INFO));
        }
        sss::colog::set_log_elements(next_style);
    }
    return int64_t(sss::colog::get_log_elements());
}

}  // namespace varlisp
