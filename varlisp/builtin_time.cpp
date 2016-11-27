#include "object.hpp"

#include "builtin_helper.hpp"
#include <sss/colorlog.hpp>

#include <sstream>

namespace varlisp {

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
    exprstr << args.head;
    COLOG_INFO("start eval ", exprstr.str());
    Object res;
    const Object& res_ref = getAtomicValue(env, args.head, res);
    COLOG_INFO("end eval ", exprstr.str());
    return res_ref;
}
} // namespace varlisp
