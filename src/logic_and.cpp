#include "logic_and.hpp"

#include <sss/utlstring.hpp>

#include "cast2bool_visitor.hpp"
#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "builtin_helper.hpp"

namespace varlisp {
Object LogicAnd::eval(Environment& env) const
{
    bool ret = true;
    for (const auto item : conditions) {
        if (!varlisp::is_true(env, item)) {
            ret = false;
            break;
        }
    }
    return ret;
}

void LogicAnd::print(std::ostream& o) const
{
    o << "(and";
    for (const auto item : conditions) {
        o << " ";
        boost::apply_visitor(print_visitor(o), item);
    }
    o << ")";
}

bool operator==(const LogicAnd& lhs, const LogicAnd& rhs) { return false; }
bool operator<(const LogicAnd& lhs, const LogicAnd& rhs) { return false; }
}  // namespace varlisp
