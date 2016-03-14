#include "condition.hpp"

#include "print_visitor.hpp"
#include "eval_visitor.hpp"
#include "environment.hpp"
#include "strict_equal_visitor.hpp"
#include "cast2bool_visitor.hpp"

#include <sss/utlstring.hpp>

namespace varlisp {
    Object Cond::eval(Environment& env) const
    {
        size_t idx = 0;
        static const varlisp::symbol sym_else = varlisp::symbol("else");
        for (const auto item : conditions) {
            if (idx == conditions.size() - 1) {
                if (const varlisp::symbol * p_v = boost::get<varlisp::symbol>(&item.first)) {
                    if (*p_v == sym_else) {
                        return boost::apply_visitor(eval_visitor(env), item.second);
                    }
                }
            }
            Object res = boost::apply_visitor(eval_visitor(env), item.first);
            bool is_condition = boost::apply_visitor(cast2bool_visitor(env), res);
            if (is_condition) {
                return boost::apply_visitor(eval_visitor(env), item.second);
            }
            ++idx;
        }
        return Object();
    }

    void Cond::print(std::ostream& o) const
    {
        o << "(cond";
        for (const auto item : conditions) {
            o << " (";
            boost::apply_visitor(print_visitor(o), item.first);
            o << " ";
            boost::apply_visitor(print_visitor(o), item.second);
            o << ")";
        }
        o << ")";
    }

    bool operator==(const Cond& lhs, const Cond& rhs)
    {
        return false;
    }
    bool operator<(const Cond& lhs, const Cond& rhs)
    {
        return false;
    }
} // namespace varlisp
