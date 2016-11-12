#include "environment.hpp"
#include "interpreter.hpp"
#include "object.hpp"

#include "builtin_helper.hpp"

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/util/PostionThrow.hpp>

namespace varlisp {

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
    Interpreter* it = env.getInterpreter();
    if (!it) {
        SSS_POSTION_THROW(std::runtime_error, "env&", &env,
                          " with nullptr Interpreter");
    }
    it->set_status(Interpreter::status_QUIT);
    return true;
}

/**
 * @brief (it:debug #t|#f) -> nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_it_debug(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "it:debug";
    Object status;
    const bool* p_status = getTypedValue<bool>(env, args.head, status);
    if (!p_status) {
        SSS_POSTION_THROW(::std::runtime_error, "(", funcName,
                          ": requires bool status at 1st argument)");
    }
    auto ll = sss::colog::get_log_levels();
    sss::colog::log_level next_ll = ll;
    if (*p_status) {
        next_ll = sss::colog::log_level((next_ll | sss::colog::ll_DEBUG) &
                                        sss::colog::ll_MASK);
    }
    else {
        next_ll = sss::colog::log_level(next_ll & ~sss::colog::ll_DEBUG &
                                        sss::colog::ll_MASK);
    }
    std::cout << SSS_VALUE_MSG(next_ll) << std::endl;

    sss::colog::set_log_levels(next_ll);
    return *p_status;
}
}  // namespace varlisp
