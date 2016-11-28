#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"

namespace varlisp {
/**
 * @brief (& int1 int2 ...) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    int ret = *p_var;
    const varlisp::List * p_list = args.next();
    while (p_list && p_list->head.which()) {
        p_var = varlisp::getTypedValue<int>(env, p_list->head, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int as parameter needed)");
        }
        ret &= *p_var;
        p_list = p_list->next();
    }
    return ret;
}

/**
 * @brief (| int1 int2 ...) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    int ret = *p_var;
    const varlisp::List * p_list = args.next();
    while (p_list && p_list->head.which()) {
        p_var = varlisp::getTypedValue<int>(env, p_list->head, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int as parameter needed)");
        }
        ret |= *p_var;
        p_list = p_list->next();
    }
    return ret;
}

/**
 * @brief (~ int) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    return ~(*p_var);
}

/**
 * @brief (^ int1 int2 ...) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, res);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    int ret = *p_var;
    const varlisp::List * p_list = args.next();
    while (p_list && p_list->head.which()) {
        p_var = varlisp::getTypedValue<int>(env, p_list->head, res);
        if (!p_var) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": int as parameter needed)");
        }
        ret ^= *p_var;
        p_list = p_list->next();
    }
    return ret;
}

/**
 * @brief (>> int shift) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, obj_arg);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }

    Object obj_shift;
    const int * p_shift = varlisp::getTypedValue<int>(env, args.tail[0].head, obj_shift);
    if (!p_shift) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    return (*p_var) >> (*p_shift);
}

/**
 * @brief (<< int shift) -> int
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
    const int * p_var = varlisp::getTypedValue<int>(env, args.head, obj_arg);
    if (!p_var) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }

    Object obj_shift;
    const int * p_shift = varlisp::getTypedValue<int>(env, args.tail[0].head, obj_shift);
    if (!p_shift) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": int as parameter needed)");
    }
    return (*p_var) << (*p_shift);
}

} // namespace varlisp
