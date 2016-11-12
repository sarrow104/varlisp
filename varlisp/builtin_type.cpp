#include "object.hpp"

#include "builtin_helper.hpp"
#include "typeid_visitor.hpp"

namespace varlisp {

/**
 * @brief
 *      (typeid type) -> id
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_typeid(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "typeid";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return boost::apply_visitor(typeid_visitor(env), obj_ref);
}

template<typename T>
bool between_co_range(const T& v, const T& min, const T& max)
{
    return min <= v && v < max;
}

template<typename T>
bool between_cc_range(const T& v, const T& min, const T& max)
{
    return min <= v && v <= max;
}

/**
 * @brief
 *      (number? type) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_number_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "number?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return between_cc_range<int>(boost::apply_visitor(typeid_visitor(env), obj_ref), 2, 3);
}

/**
 * @brief
 *      (boolean? type) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_boolean_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "number?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return  boost::apply_visitor(typeid_visitor(env), obj_ref) == typeid_visitor(env)(true);
}

/**
 * @brief
 *      (string? type) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_string_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "number?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return boost::apply_visitor(typeid_visitor(env), obj_ref) == typeid_visitor(env)(std::string{});
}

/**
 * @brief
 *      (number? type) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_slist_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "slist?";
    Object obj;
    const varlisp::List * p_list = getFirstListPtrFromArg(env, args, obj);
    return p_list != 0;
}

} // namespace varlisp
