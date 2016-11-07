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
Object eval_eq(varlisp::Environment& env, const varlisp::List& args)
{
    return Object(boost::apply_visitor(strict_equal_visitor(env),
                                       args.head,
                                       args.tail[0].head));
}

Object eval_gt(varlisp::Environment& env, const varlisp::List& args)
{
    return Object(
        !boost::apply_visitor(strict_equal_visitor(env),
                              args.tail[0].head,
                              args.head) &&
        boost::apply_visitor(strict_less_visitor(env),
                             args.tail[0].head,
                             args.head));
}

Object eval_lt(varlisp::Environment& env, const varlisp::List& args)
{
    return Object(boost::apply_visitor(strict_less_visitor(env),
                                       args.head,
                                       args.tail[0].head));
}

Object eval_ge(varlisp::Environment& env, const varlisp::List& args)
{
    return Object(boost::apply_visitor(strict_less_visitor(env),
                                       args.tail[0].head,
                                       args.head));
}

Object eval_le(varlisp::Environment& env, const varlisp::List& args)
{
    return Object(
        boost::apply_visitor(strict_equal_visitor(env),
                             args.tail[0].head,
                             args.head)
        ||
        boost::apply_visitor(strict_less_visitor(env),
                             args.head,
                             args.tail[0].head));

}

} // namespace varlisp

