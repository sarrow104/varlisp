#include <array>

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

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
    const auto * p_var =
        requireTypedValue<int64_t>(env, args.nth(0), res, funcName, 0, DEBUG_INFO);

    int64_t ret = *p_var;
    for (size_t i = 1; i < args.length(); ++i) {
        ret &= *requireTypedValue<int64_t>(env, args.nth(i), res, funcName, i, DEBUG_INFO);
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
    const auto * p_var =
        requireTypedValue<int64_t>(env, args.nth(0), res, funcName, 0, DEBUG_INFO);

    int64_t ret = *p_var;
    for (size_t i = 1; i < args.length(); ++i) {
        ret |= *requireTypedValue<int64_t>(env, args.nth(i), res, funcName, i, DEBUG_INFO);
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
    return ~(*requireTypedValue<int64_t>(env, args.nth(0), res, funcName, 0, DEBUG_INFO));
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
    const auto * p_var =
        requireTypedValue<int64_t>(env, args.nth(0), res, funcName, 0, DEBUG_INFO);

    int64_t ret = *p_var;
    for (size_t i = 1; i < args.length(); ++i) {
        ret ^= *requireTypedValue<int64_t>(env, args.nth(i), res, funcName, i, DEBUG_INFO);
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
    std::array<Object, 2> objs;
    const auto * p_var   = requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    const auto * p_shift = requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
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
    std::array<Object, 2> objs;
    const auto * p_var   = requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    const auto * p_shift = requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
    return (*p_var) << (*p_shift);
}

} // namespace varlisp
