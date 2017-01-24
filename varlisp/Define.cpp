#include "Define.hpp"

#include <sss/colorlog.hpp>

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "cast2bool_visitor.hpp"
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
    bool write_value = true;
    // TODO FIXME Environment::find()使用的是map的find，即，标识符，按整体查找。
    // 因此，不支持json-accessor模式。
    // 这里与一般的语义不符，需要修改。
    // 但是，后续赋值的时候，又使用的是operator[] ——这个是json-accessor版的！
    // 两者矛盾。
    if (top_env->find(this->name.name())) {
        Object tmp;
        const auto& forceRefer = getAtomicValue(env, this->force_rewrite, tmp);
        bool force = boost::apply_visitor(cast2bool_visitor(env), forceRefer);
        if (!force) {
            write_value = false;
            COLOG_ERROR("cannot redefine ", this->name);
        }
        else {
            COLOG_INFO("force redefine ", this->name);
        }
    }

    if (write_value)
    {
        const Object& resRef = getAtomicValue(env, this->value, tmp);

        COLOG_DEBUG(this->name.name(), resRef);

        // FIXME binding
        top_env->operator[](this->name.name()) = resRef;
    }
    // NOTE 为什么不再返回value呢？
    // 为了减少显示；
    // 既然是定义变量，再链式赋值，也意义不大了。这里，沿用clisp习惯，返回定义的
    // 标识符
    return this->name;
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
