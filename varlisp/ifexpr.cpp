#include "ifexpr.hpp"

#include <sss/utlstring.hpp>

#include "cast2bool_visitor.hpp"
#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
// NOTE 关于if
//
// > (if #t 5 6)
// 5
// > (if (list) 5 6)
// 5
// > (if (+) 5 6)
// 5
// > (if (0) 5 6)
// application: not a procedure;
//  expected a procedure that can be applied to arguments
//   given: 0
//   arguments...: [none]
// > (if (-) 5 6)
// -: arity mismatch;
//  the expected number of arguments does not match the given number
//   expected: at least 1
//   given: 0
// > (if 0 5 6)
// 5
//
// if 对于 #t,#f的判断，有别于其他程序语言，甚至不同的lisp方言，也是不一样的；
// 上面是DrRacket的认知；
//
// 而Lisp中，只有 '()空列表（nil）是假，其余都是真！
void IfExpr::print(std::ostream& o) const
{
    o << "(if ";
    boost::apply_visitor(print_visitor(o), this->condition);
    o << " ";
    boost::apply_visitor(print_visitor(o), this->consequent);
    o << " ";
    boost::apply_visitor(print_visitor(o), this->alternative);
    o << ")";
}

Object IfExpr::eval(Environment& env) const
{
    Object res = boost::apply_visitor(eval_visitor(env), this->condition);

    bool is_condition = boost::apply_visitor(cast2bool_visitor(env), res);

    if (is_condition) {
        return boost::apply_visitor(eval_visitor(env), this->consequent);
    }
    else {
        return boost::apply_visitor(eval_visitor(env), this->alternative);
    }
}

// 不可比较
bool operator==(const IfExpr& lhs, const IfExpr& rhs)
{
    // return
    //     boost::apply_visitor(strict_equal_visitor(), lhs.condition,
    //     rhs.condition) &&
    //     boost::apply_visitor(strict_equal_visitor(), lhs.consequent,
    //     rhs.consequent) &&
    //     boost::apply_visitor(strict_equal_visitor(), lhs.alternative,
    //     rhs.alternative);
    return false;
}

bool operator<(const IfExpr& lhs, const IfExpr& rhs) { return false; }
}  // namespace varlisp
