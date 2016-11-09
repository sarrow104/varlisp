#include "object.hpp"

#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"

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
    return Object(boost::apply_visitor(strict_equal_visitor(env), args.head,
                                       args.tail[0].head));
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
    return Object(!boost::apply_visitor(strict_equal_visitor(env),
                                        args.tail[0].head, args.head) &&
                  boost::apply_visitor(strict_less_visitor(env),
                                       args.tail[0].head, args.head));
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
    return Object(boost::apply_visitor(strict_less_visitor(env), args.head,
                                       args.tail[0].head));
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
    return Object(!boost::apply_visitor(strict_less_visitor(env), args.head,
                                       args.tail[0].head));
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
    return Object(boost::apply_visitor(strict_equal_visitor(env),
                                       args.tail[0].head, args.head) ||
                  boost::apply_visitor(strict_less_visitor(env), args.head,
                                       args.tail[0].head));
}

// TODO (! ) (not ) 当callable，为#f的时候，返回#t；反之，返回#f；
// 需要注意的是，由于只有#t才判定为真，

}  // namespace varlisp
