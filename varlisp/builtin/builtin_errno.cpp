#include <string.h>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("errno", 0, 0, eval_errno, "(errno) -> int64_t");

/**
 * @brief (errno) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_errno(varlisp::Environment& env, const varlisp::List& args)
{
    return int64_t(errno);
}

REGIST_BUILTIN("strerr", 1, 1, eval_strerr, "(strerr) -> string");

/**
 * @brief (strerr) -> string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_strerr(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "strerr";
    Object obj;
    const int64_t* p_err =
        requireTypedValue<int64_t>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);

#if 0
    char buf[128] = "\0";
    ::strerror_r(*p_err, buf, sizeof(buf));
    return varlisp::string_t{std::string(buf)};
#else
    return varlisp::string_t{std::string(::strerror(*p_err))};
#endif
}

} // namespace varlisp
