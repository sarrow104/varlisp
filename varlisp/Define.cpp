#include "Define.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

#include "environment.hpp"

namespace varlisp {
void Define::print(std::ostream& o) const
{
    o << "(define " << this->name << " ";
    boost::apply_visitor(print_visitor(o), this->value);
    o << ")";
}

Object Define::eval(Environment& env) const
{
    Object value = boost::apply_visitor(eval_visitor(env), this->value);
    env[this->name.m_data] = value;
    return value;
}

// 不可比较！
bool operator==(const Define& lhs, const Define& rhs)
{
    //         return lhs.name == rhs.name &&
    //             boost::apply_visitor(strict_equal_visitor(), lhs.value,
    //             rhs.value);
    return false;
}

// 不可比较！
bool operator<(const Define& lhs, const Define& rhs) { return false; }
}  // namespace varlisp
