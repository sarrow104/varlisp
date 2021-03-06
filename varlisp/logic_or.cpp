#include "logic_or.hpp"

#include <sss/utlstring.hpp>

#include "cast2bool_visitor.hpp"
#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "builtin_helper.hpp"

namespace varlisp {
Object LogicOr::eval(Environment& env) const
{
    bool ret = false;
    for (const auto item : conditions) {
        if (varlisp::is_true(env, item)) {
            ret = true;
            break;
        }
    }
    return ret;
}

void LogicOr::print(std::ostream& o) const
{
    o << "(or";
    for (const auto item : conditions) {
        o << " ";
        boost::apply_visitor(print_visitor(o), item);
    }
    o << ")";
}

bool operator==(const LogicOr& lhs, const LogicOr& rhs) { return false; }
bool operator<(const LogicOr& lhs, const LogicOr& rhs) { return false; }
}  // namespace varlisp
