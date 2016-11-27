#include "object.hpp"
#include "builtin_helper.hpp"

#include <string.h>

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

} // namespace varlisp
