#include <string.h>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"

namespace varlisp {

/**
 * @brief (errno) -> int
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_errno(varlisp::Environment& env, const varlisp::List& args)
{
    return errno;
}

REGIST_BUILTIN("errno", 0, 0, eval_errno, "(errno) -> int");

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
    const int* p_err =
        getTypedValue<int>(env, args.head, obj);
    if (!p_err) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int errnor as 1st argument)");
    }
#if 0
    char buf[128] = "\0";
    ::strerror_r(*p_err, buf, sizeof(buf));
    return varlisp::string_t{std::string(buf)};
#else
    return varlisp::string_t{std::string(::strerror(*p_err))};
#endif
}

REGIST_BUILTIN("strerr", 1, 1, eval_strerr, "(strerr) -> string");

} // namespace varlisp
