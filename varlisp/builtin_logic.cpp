#include "object.hpp"

#include "builtin_helper.hpp"
#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"

#include "builtin_helper.hpp"

namespace varlisp {
// 对于drracket来说，比较运算符，它会直接要求转换到real域；
// 如果失败，则抛出异常；
// 即，lambda函式，与1.2实数的比较大小，会出错，抛出异常：
// > (if (> fib 1) 1 2)
// >: contract violation
//   expected: real?
//   given: #<procedure:fib>
//   argument position: 1st
//   other arguments...:
//    1
// 其中：
// > (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))
/**
 * @brief (= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_eq(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, args.head, obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, args.tail[0].head, obj2);

    return Object(boost::apply_visitor(strict_equal_visitor(env), obj1_ref, obj2_ref));
}

/**
 * @brief (> obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gt(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, args.head, obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, args.tail[0].head, obj2);
    return Object(!boost::apply_visitor(strict_equal_visitor(env), obj2_ref, obj1_ref) &&
                  boost::apply_visitor(strict_less_visitor(env), obj2_ref, obj1_ref));
}

/**
 * @brief (< obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_lt(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, args.head, obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, args.tail[0].head, obj2);
    return Object(boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref));
}

/**
 * @brief (>= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ge(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, args.head, obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, args.tail[0].head, obj2);
    return Object(!boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref));
}

/**
 * @brief (<= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_le(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, args.head, obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, args.tail[0].head, obj2);
    return Object(boost::apply_visitor(strict_equal_visitor(env), obj2_ref, obj1_ref) ||
                  boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref));
}

// TODO (! ) (not ) 当callable，为#f的时候，返回#t；反之，返回#f；
// 需要注意的是，由于只有#t才判定为真，

/**
 * @brief (not expr) -> !#t | !#f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_not(varlisp::Environment& env, const varlisp::List& args)
{
    return !varlisp::is_true(env, args.head);
}

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

/**
 * @brief
 *    (equal '(list1) '(list2)) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_equal(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const varlisp::List * p_list1 = varlisp::getFirstListPtrFromArg(env, args, obj1);
    if (!p_list1) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(equal: 1st argument must be an s-list)");
    }
    Object obj2;
    const varlisp::List * p_list2 = varlisp::getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list2) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(equal: 1st argument must be an s-list)");
    }
    p_list1 = p_list1->next();
    p_list2 = p_list2->next();
    bool is_equal = true;
    if (p_list1->length() == p_list2->length()) {
        while (p_list2 && p_list2) {
            if (!boost::apply_visitor(strict_equal_visitor(env),
                                    p_list1->head,
                                    p_list2->head)) {
                is_equal = false;
                break;
            }
            p_list1 = p_list1->next();
            p_list2 = p_list2->next();
        }
    }
    else {
        is_equal = false;
    }
    return is_equal;
}

}  // namespace varlisp
