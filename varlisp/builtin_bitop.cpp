#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("&", 2, -1, eval_bit_and, "(& int1 int2 ...) -> int64_t");
/**
 * @brief (& int1 int2 ...) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_and(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "&";
    Object res;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    int64_t ret = *p_var;
    const varlisp::List tail = args.tail();
    for (auto it = tail.begin(); it != tail.end(); ++it) {
        p_var = varlisp::getTypedValue<int64_t>(env, *it, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int64_t as parameter needed)");
        }
        ret &= *p_var;
    }
    return ret;
}

REGIST_BUILTIN("|", 2, -1, eval_bit_or, "(| int1 int2 ...) -> int64_t");

/**
 * @brief (| int1 int2 ...) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_or(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "|";
    Object res;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    int64_t ret = *p_var;
    const varlisp::List tail = args.tail();
    for (auto it = tail.begin(); it != tail.end(); ++it) {
        p_var = varlisp::getTypedValue<int64_t>(env, *it, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int64_t as parameter needed)");
        }
        ret |= *p_var;
    }
    return ret;
}

REGIST_BUILTIN("~", 1, 1, eval_bit_rev, "(~ int64_t) -> int64_t");

/**
 * @brief (~ int64_t) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_rev(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "~";
    Object res;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    return ~(*p_var);
}

REGIST_BUILTIN("^", 2, -1, eval_bit_xor, "(^ int1 int2 ...) -> int64_t");

/**
 * @brief (^ int1 int2 ...) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_xor(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "^";
    Object res;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    int64_t ret = *p_var;
    const varlisp::List tail = args.tail();
    for (auto it = tail.begin(); it != tail.end(); ++it) {
        p_var = varlisp::getTypedValue<int64_t>(env, *it, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int64_t as parameter needed)");
        }
        ret ^= *p_var;
    }
    return ret;
}

REGIST_BUILTIN(">>", 2, 2, eval_bit_shift_right, "(>> int64_t shift) -> int64_t");

/**
 * @brief (>> int64_t shift) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_shift_right(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = ">>";
    Object obj_arg;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), obj_arg);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }

    Object obj_shift;
    const int64_t * p_shift = varlisp::getTypedValue<int64_t>(env, detail::cadr(args), obj_shift);
    if (!p_shift) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    return (*p_var) >> (*p_shift);
}

REGIST_BUILTIN("<<", 2, 2, eval_bit_shift_left, "(<< int64_t shift) -> int64_t");

/**
 * @brief (<< int64_t shift) -> int64_t
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_bit_shift_left(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "<<";
    Object obj_arg;
    const int64_t * p_var = varlisp::getTypedValue<int64_t>(env, detail::car(args), obj_arg);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }

    Object obj_shift;
    const int64_t * p_shift = varlisp::getTypedValue<int64_t>(env, detail::cadr(args), obj_shift);
    if (!p_shift) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int64_t as parameter needed)");
    }
    return (*p_var) << (*p_shift);
}

} // namespace varlisp
