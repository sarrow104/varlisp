#include "Define.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "builtin_helper.hpp"

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
    Object tmp;
    env[this->name.m_data] = getAtomicValue(env, this->value, tmp);
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
