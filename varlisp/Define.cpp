#include "Define.hpp"

#include <sss/colorlog.hpp>

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

// NOTE execute when this->name not declared yet!
Object Define::eval(Environment& env) const
{
    Object tmp;
    Environment * top_env = env.ceiling();
    if (top_env->find(this->name.m_data)) {
        return Nill{};
    }
    const Object& resRef = getAtomicValue(env, this->value, tmp);

    COLOG_DEBUG(this->name.m_data, resRef);

    top_env->operator[](this->name.m_data)
        = resRef;
    // 为什么不再返回value呢？
    // 为例减少显示；
    // 既然是定义了变量，链式赋值，也意义不大了
    return Object{Nill{}};
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
