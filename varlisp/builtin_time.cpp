#include <sstream>

#include <sss/colorlog.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("time", 1, 1, eval_time, "(time expr) -> result-of-expr");

/**
 * @brief
 *      (time expr) -> result-of-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_time(varlisp::Environment &env, const varlisp::List &args)
{
    std::ostringstream exprstr;
    exprstr << detail::car(args);
    COLOG_INFO("start eval ", exprstr.str());
    Object res;
    const Object& res_ref = getAtomicValue(env, detail::car(args), res);
    COLOG_INFO("end eval ", exprstr.str());
    return res_ref;
}

} // namespace varlisp
