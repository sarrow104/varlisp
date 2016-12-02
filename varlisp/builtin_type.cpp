#include "object.hpp"

#include "builtin_helper.hpp"
#include "cast_visitor.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/is_symbol.hpp"

namespace varlisp {

REGIST_BUILTIN("typeid", 1, 1, eval_typeid, "(typeid expr) -> integar");

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
    // const char * funcName = "typeid";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return varlisp::typedid(env, obj_ref);
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

REGIST_BUILTIN("number?", 1, 1, eval_number_q, "(number? expr) -> boolean");

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
    // const char * funcName = "number?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return between_cc_range<int>(varlisp::typedid(env, obj_ref),
                                 varlisp::typedid(env, varlisp::Object{1}),
                                 varlisp::typedid(env, varlisp::Object{1.0}));
}

REGIST_BUILTIN("boolean?", 1, 1, eval_boolean_q, "(boolean? expr) -> boolean");

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
    // const char * funcName = "boolean?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return varlisp::typedid(env, obj_ref) ==
           varlisp::typedid(env, Object{true});
}

REGIST_BUILTIN("string?",         1,  1,  eval_string_q, "(string? expr) -> boolean");

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
    // const char * funcName = "string?";
    Object obj;
    const Object& obj_ref = getAtomicValue(env, args.head, obj);
    return varlisp::typedid(env, obj_ref) ==
           varlisp::typedid(env, Object{varlisp::string_t{}});
}

REGIST_BUILTIN("slist?", 1, 1, eval_slist_q, "(slist? expr) -> boolean");

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
    // const char * funcName = "slist?";
    Object obj;
    const varlisp::List * p_list = getFirstListPtrFromArg(env, args, obj);
    return p_list != 0;
}

REGIST_BUILTIN("null?", 1, 1, eval_null_q, "(null? expr) -> boolean");

/**
 * @brief
 *    (null? nil) -> #t
 *    (null? (not nil)) -> #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_null_q(varlisp::Environment& env, const varlisp::List& args)
{
    Object tmp;
    const Object& obj = getAtomicValue(env, args.head, tmp);
    return obj.which() == 1;
}

REGIST_BUILTIN("cast", 2, 2, eval_cast,
               "(cast lexical-value var) -> type-of-lexical-value");

/**
 * @brief
 *    (cast lexical-value var) -> type-of-lexical-value
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_cast(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "cast";
    Object tmp;
    const varlisp::List * p_1st = &args;
    const varlisp::List * p_2nd = p_1st->next();

    return boost::apply_visitor(cast_visitor(env, p_1st->head, p_2nd->head),
                                p_1st->head, p_2nd->head);
}

} // namespace varlisp
