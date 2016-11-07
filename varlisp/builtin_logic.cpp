#include "object.hpp"

#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"

namespace varlisp {
// ����drracket��˵���Ƚ������������ֱ��Ҫ��ת����real��
// ���ʧ�ܣ����׳��쳣��
// ����lambda��ʽ����1.2ʵ���ıȽϴ�С��������׳��쳣��
// > (if (> fib 1) 1 2)
// >: contract violation
//   expected: real?
//   given: #<procedure:fib>
//   argument position: 1st
//   other arguments...:
//    1
// ���У�
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

